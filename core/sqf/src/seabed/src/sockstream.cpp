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

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <netinet/in.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include "seabed/fserr.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "imap.h"
#include "lmap.h"
#include "llmap.h"
#include "mserr.h"
#include "msmon.h"
#include "mstrace.h"
#include "msutil.h"
#include "msx.h"
#include "socktrans.h"
#include "util.h"
#include "utracex.h"


#ifdef USE_SEND_LAT
enum {                 MAX_LAT = 1000 };
static struct timeval  ga_stream_lat_elapsed[MAX_LAT];
static int             gv_stream_lat_inx = 0;
#endif

typedef struct SS_Node {
    SB_ML_Type             iv_link;
    SB_Trans::Sock_Stream *ip_stream;
} SS_Node;

SB_Trans::Sock_Listener *SB_Trans::Sock_Stream::cp_listener = NULL;
bool                     SB_Trans::Sock_Stream::cv_mon_stream_set = false;
SB_Ts_Imap               SB_Trans::Sock_Stream::cv_stream_map;

SB_Trans::Sock_Stream_Accept_Thread *SB_Trans::Sock_Stream::cp_accept_thread = NULL;
SB_Trans::Sock_Stream_Helper_Thread *SB_Trans::Sock_Stream::cp_helper_thread = NULL;

//
// Purpose: create SS node (socket-stream)
//
static SS_Node *new_SS_Node(int                    pv_sock,
                            SB_Trans::Sock_Stream *pp_stream) {
    const char *WHERE = "new_SS_Node";

    SS_Node *lp_node = new SS_Node();
    lp_node->iv_link.iv_id.i = pv_sock;
    lp_node->ip_stream = pp_stream;
    pp_stream->ref_inc(WHERE, &lp_node->ip_stream);
    return lp_node;
}

static void sb_trans_sock_stream_setup_signals() {
    int      lv_err;
    sigset_t lv_set;
    int      lv_sig;

    // TODO: fix this
    // hack to turn off local-io signals
    lv_sig = SIGRTMAX - 4;
    sigemptyset(&lv_set);
    sigaddset(&lv_set, lv_sig);
    lv_err = pthread_sigmask(SIG_BLOCK, &lv_set, NULL);
    SB_util_assert_ieq(lv_err, 0); // sw fault
}

//
// Purpose: constructor socket stream
//
SB_Trans::Sock_Stream::Sock_Stream()
: Trans_Stream(STREAM_TYPE_SOCK), iv_aecid_sock_stream(SB_ECID_STREAM_SOCK) {
}

//
// Purpose: constructor socket stream
//
SB_Trans::Sock_Stream::Sock_Stream(const char           *pp_name,
                                   const char           *pp_pname,
                                   const char           *pp_prog,
                                   bool                  pv_ic,
                                   Sock_User            *pp_sock,
                                   SB_Mon_Cb_Type,
                                   SB_Mon_Cb_Type,
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
: Trans_Stream(STREAM_TYPE_SOCK,
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
               pv_open_nid,
               pv_open_pid,
#ifdef SQ_PHANDLE_VERIFIER
               pv_open_verif,
#endif
               pv_opened_nid,
               pv_opened_pid,
#ifdef SQ_PHANDLE_VERIFIER
               pv_opened_verif,
#endif
               pv_opened_type),
  iv_aecid_sock_stream(SB_ECID_STREAM_SOCK),
  ip_sock(pp_sock),
  iv_close_ind_sent(false),
  iv_seq(1),
  iv_sock_errored(0),
  iv_stopped(false) {
    const char *WHERE = "Sock_Stream::Sock_Stream";
    if (strcmp(pp_name, "monitor") == 0)
        ref_inc(WHERE, NULL); // death expects a reference
    ip_sock_eh = new Sock_Stream_EH(this);
    if (ip_sock == NULL)
        iv_sock = -1;
    else
        iv_sock = ip_sock->get_sock();
    iv_r.iv_state = RSTATE_INIT;
    iv_r.iv_section = RSECTION_HDR;
    if (pv_open_nid < 0) {
        iv_remote_nid = pv_opened_nid;
        iv_remote_pid = pv_opened_pid;
#ifdef SQ_PHANDLE_VERIFIER
        iv_remote_verif = pv_opened_verif;
#endif
    } else {
        iv_remote_nid = pv_open_nid;
        iv_remote_pid = pv_open_pid;
#ifdef SQ_PHANDLE_VERIFIER
        iv_remote_verif = pv_open_verif;
#endif
    }
    SS_Node *lp_node = new_SS_Node(iv_sock, this);
    cv_stream_map.put(&lp_node->iv_link);
    if (pv_open_nid >= 0)
        map_nidpid_add_stream(pv_open_nid, pv_open_pid, pv_open_verif, false);

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "creating this=%p\n", pfp(this));
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "bind sock-user this=%p, sock=%p\n",
                           pfp(this), pfp(ip_sock));
}

//
// Purpose: destructor socket stream
//
SB_Trans::Sock_Stream::~Sock_Stream() {
    const char *WHERE = "Sock_Stream::~Sock_Stream";
    bool        lv_marker;

    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "destroying this=%p steam=%p\n", pfp(this), pfp(this));

    sock_lock();
    if (ip_sock != NULL) {
        ip_sock->destroy();
        ip_sock = NULL;
    }
    sock_unlock();
    lv_marker = get_thread_marker();
    if (!lv_marker)
        ip_sock_eh->eh_lock();
    ip_sock_eh->set_stream(NULL);
    if (!lv_marker)
        ip_sock_eh->eh_unlock();
}

//
// Purpose: close sock
//
void SB_Trans::Sock_Stream::close_sock() {
    iv_sock_errored = EHOSTDOWN;
    sock_lock();
    if (ip_sock != NULL)
        ip_sock->stop();
    sock_unlock();
    if (iv_open_nid >= 0) { // accept
        if (!iv_close_ind_sent) {
            iv_close_ind_sent  = true;
            send_close_ind();
        }
    }
    SS_Node *lp_node = static_cast<SS_Node *>(cv_stream_map.remove(iv_sock));
    if (lp_node != NULL) {
        lp_node->ip_stream->ref_dec("close_sock:delete_SS_Node",
                                    &lp_node->ip_stream);
    }
    delete lp_node;

#ifdef USE_SEND_LAT
    for (int lv_inx = 0; lv_inx < gv_stream_lat_inx; lv_inx++)
        printf("%d: sendlat[%d]=%ld.%06ld\n",
               getpid(),
               lv_inx,
               ga_stream_lat_elapsed[lv_inx].tv_sec,
               ga_stream_lat_elapsed[lv_inx].tv_usec);
#endif
}

//
// Purpose: close stream
//
void SB_Trans::Sock_Stream::close_stream(Sock_Stream *pp_stream,
                                         bool         pv_local) {
    pp_stream->close_this(pv_local, true, false);
}

//
// Purpose: close streams
//
void SB_Trans::Sock_Stream::close_streams(bool pv_join) {
    const char *WHERE = "Sock_Stream::close_streams";
    int         lv_status;

    while (cv_stream_map.size()) {
        cv_stream_map.lock();
        SB_Imap_Enum *lp_enum = cv_stream_map.keys();
        enum { MAX_STREAMS = 10 };
        int la_socks[MAX_STREAMS];
        int lv_max;
        for (lv_max = 0;
             lp_enum->more() && (lv_max < MAX_STREAMS);
             lv_max++)
            la_socks[lv_max] = lp_enum->next()->iv_id.i;
        cv_stream_map.unlock();
        for (int lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            SS_Node *lp_node =
              static_cast<SS_Node *>(cv_stream_map.remove(la_socks[lv_inx]));
            if (lp_node != NULL) {
                if (gv_ms_trace_sock)
                    trace_where_printf(WHERE, "removed sock=%d, stream=%p %s\n",
                                       lp_node->iv_link.iv_id.i,
                                       pfp(lp_node->ip_stream),
                                       lp_node->ip_stream->ia_stream_name);
                close_stream(lp_node->ip_stream, true);
            }
            delete lp_node;
        }
        delete lp_enum;
    }

    if (cp_accept_thread != NULL) {
        void *lp_result;
        if (gv_ms_trace_sock)
            trace_where_printf(WHERE, "shutdown accept thread\n");
        cp_accept_thread->fin();
        if (pv_join) {
            lv_status = cp_accept_thread->join(&lp_result);
            if (gv_ms_trace_sock)
                trace_where_printf(WHERE, "accept thread death, status=%d\n",
                                   lv_status);
            SB_util_assert_ieq(lv_status, 0);
        }
        cp_accept_thread = NULL;
    }
    if (cp_helper_thread != NULL) {
        void *lp_result;
        cp_helper_thread->fin();
        if (gv_ms_trace)
            trace_where_printf(WHERE, "waiting for helper thread death\n");
        lv_status = cp_helper_thread->join(&lp_result);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "helper thread death ack, status=%d\n",
                               lv_status);
        SB_util_assert_ieq(lv_status, 0);
        cp_helper_thread = NULL;
    }
    Sock_Controller::shutdown(WHERE);
}

//
// Purpose: close stream
//
void SB_Trans::Sock_Stream::close_this(bool pv_local,
                                       bool pv_lock,
                                       bool pv_sem) {
    const char *WHERE = "Sock_Stream::close_this";

    pv_local = pv_local; // touch
    pv_sem = pv_sem; // touch
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "ENTER stream=%p %s\n",
                           pfp(this), ia_stream_name);
    if (!iv_stopped) {
        iv_stopped = true;

        // clear nidpid map
        if (iv_open_nid >= 0)
            map_nidpid_remove(pv_lock);
        close_sock();
        if (md_ref_get() == 0)
            sock_free();

        // change counts last
        if (iv_open_nid < 0)
            add_stream_con_count(-1);
        else
            add_stream_acc_count(-1);
    }
}

//
// Purpose: complete sends
//
void SB_Trans::Sock_Stream::comp_sends() {
}

//
// Purpose: create stream
//
SB_Trans::Sock_Stream *SB_Trans::Sock_Stream::create() {
    Sock_Stream *lp_stream = new Sock_Stream();
    return lp_stream;
}

//
// Purpose: create socket stream
//
SB_Trans::Sock_Stream *
SB_Trans::Sock_Stream::create(const char           *pp_name,
                              const char           *pp_pname,
                              const char           *pp_prog,
                              bool                  pv_ic,
                              Sock_User            *pp_sock,
                              SB_Mon_Cb_Type        pv_mon_callback,
                              SB_Mon_Cb_Type        pv_mon_unsol_callback,
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
                              int                   pv_opened_type) {
    Sock_Stream *lp_stream =
      new Sock_Stream(pp_name,
                      pp_pname,
                      pp_prog,
                      pv_ic,
                      pp_sock,
                      pv_mon_callback,
                      pv_mon_unsol_callback,
                      pv_ms_comp_callback,
                      pv_ms_abandon_callback,
                      pv_ms_oc_callback,
                      pv_ms_lim_callback,
                      pp_ms_recv_q,
                      pp_ms_lim_q,
                      pp_event_mgr,
                      pv_open_nid,
                      pv_open_pid,
#ifdef SQ_PHANDLE_VERIFIER
                      pv_open_verif,
#endif
                      pv_opened_nid,
                      pv_opened_pid,
#ifdef SQ_PHANDLE_VERIFIER
                      pv_opened_verif,
#endif
                      pv_opened_type);
    if (pp_sock != NULL) {
        pp_sock->set_nonblock();
        pp_sock->event_init(lp_stream->ip_sock_eh);
    }
    SB_Trans::Trans_Stream::delete_streams(true);
    return lp_stream;
}

//
// Purpose: process epoll events
//
void SB_Trans::Sock_Stream::process_events(int pv_events) {
    const char *WHERE = "Sock_Stream::process_events";
    enum      { EVENTS_ERR = POLLERR | POLLHUP };
    static const char *la_section[] = {
        "HDR",
        "CTRL",
        "DATA"
    };
    static const char *la_state[] = {
        "INIT",
        "RCV",
        "RCVING",
        "RCVD",
        "ERROR",
        "EOF"
    };
    MS_PMH_Type  *lp_hdr;
    MS_Md_Type   *lp_md = NULL; // make coverity happy
    RInfo_Type   *lp_r;
    Rd_Type      *lp_rd;
    const char   *lp_section = NULL; // make compiler happy
    const char   *lp_state = NULL; // make compiler happy
    bool          lv_client;
    bool          lv_cont;
    int           lv_len;

    if (!cv_mon_stream_set) {
        cv_mon_stream_set = true;
        set_thread_marker(true);
    }

    lp_r = &iv_r;        // ref iv_r through lp_r
    lp_rd = &lp_r->iv_rd;
    lp_hdr = &lp_rd->iv_hdr;
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "stream=%p, events=0x%x\n", pfp(this), pv_events);
    if (pv_events & (POLLIN | EVENTS_ERR)) {
        lv_cont = true;
        while (lv_cont) {
            lv_cont = false;

            if (gv_ms_trace_sm) {
                if ((lp_r->iv_state >= RSTATE_INIT) &&
                    (lp_r->iv_state < RSTATE_LAST))
                    lp_state = la_state[lp_r->iv_state];
                else
                    lp_state = "<unknown>";
                if ((lp_r->iv_section >= RSECTION_HDR) &&
                    (lp_r->iv_section <= RSECTION_DATA))
                    lp_section = la_section[lp_r->iv_section];
                else
                    lp_section = "<unknown>";
                trace_where_printf(WHERE, "sock=%d, state=%d(%s), section=%d(%s)\n",
                                   iv_sock,
                                   lp_r->iv_state, lp_state,
                                   lp_r->iv_section, lp_section);
            }
            switch (lp_r->iv_state) {
            case RSTATE_INIT:
                lp_r->iv_rd.ip_where = WHERE;
                lp_r->iv_state = RSTATE_RCV;
                lv_cont = true;
                break;

            case RSTATE_RCV:
                lp_r->iv_section = RSECTION_HDR;
                recv_buf_init(lp_r,
                              lp_hdr,
                              MS_PMH_HDR_SIZE,
                              &lv_cont);
                break;

            case RSTATE_RCVING:
                recv_buf_cont(lp_r, &lv_cont);
                break;

            case RSTATE_RCVD:
                switch (lp_r->iv_section) {
                case RSECTION_HDR:
                    process_hdr(&lp_md, &lv_cont);
                    // save in case EWOULDBLOCK
                    lp_r->ip_md = lp_md;
                    break;

                case RSECTION_CTRL:
                    lp_r->iv_section = RSECTION_DATA;
                    lv_len = lp_hdr->iv_dsize;
                    lp_rd->iv_data_len = lv_len;
                    lp_md = lp_r->ip_md;
                    process_ctrl(lp_md, &lv_cont);
                    break;

                case RSECTION_DATA:
                    switch (lp_hdr->iv_type) {
                    case MS_PMH_TYPE_ABANDON_ACK:
                    case MS_PMH_TYPE_CLOSE_ACK:
                    case MS_PMH_TYPE_OPEN_ACK:
                    case MS_PMH_TYPE_REPLY:
                    case MS_PMH_TYPE_REPLY_NACK:
                        lv_client = true;
                        break;
                    default:
                        lv_client = false;
                        break;
                    }
                    finish_recv(&lp_r->iv_rd, &lp_r->iv_rd.iv_hdr, lv_client);
                    lp_r->iv_state = RSTATE_RCV;
                    lv_cont = true;
                    break;
                }
                break;

            case RSTATE_ERROR:
            case RSTATE_EOF:
//sleep(2);
                finish_writereads(ms_err_sock_to_fserr(WHERE,
                                  iv_sock_errored));
                stop_recv();
                if (!cv_shutdown && (iv_open_nid >= 0)) {
                    if (map_nidpid_remove(true)) { // it's helper2's now
                        Trans_Stream::close_stream(WHERE, this, true, true, true);
                    }
                }
                break;

            default:
                SB_util_abort("invalid lp_r->iv_state"); // sw fault
                break;
            }
        }
    }
    if (pv_events & POLLOUT) {
        send_sm();
    }
}

//
// Purpose: execute functions
//
short SB_Trans::Sock_Stream::exec_abandon(MS_Md_Type *pp_md,
                                          int         pv_reqid,
                                          int         pv_can_reqid) {
    const char *WHERE = "Sock_Stream::exec_abandon";
    short       lv_fserr;
    int         lv_sockerr;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d, can-reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid, pv_can_reqid);
    exec_com_init_simple(pp_md, pv_reqid, pv_can_reqid);
    lv_fserr = exec_com_chk(WHERE);
    if (gv_ms_trace_abandon)
        trace_where_printf(WHERE, "reqid=%d, can-reqid=%d, fserr=%d\n",
                           pv_reqid, pv_can_reqid, lv_fserr);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_ABANDON, pp_md, lv_fserr);
        lv_fserr = 0; // clear error
        finish_abandon(pp_md);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
        return lv_fserr;
    }

    iv_req_map.put(pp_md);
    lv_sockerr = send_md(MS_OP_ABANDON,
                         MS_PMH_TYPE_ABANDON,
                         MD_STATE_ABANDON_SENDING,
                         0,
                         pp_md,
                         XZFIL_ERR_OK);
    if (lv_sockerr)
        iv_req_map.remove(pp_md->iv_link.iv_id.i);
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
    return lv_fserr;
}

short SB_Trans::Sock_Stream::exec_abandon_ack(MS_Md_Type *pp_md,
                                              int         pv_reqid,
                                              int         pv_can_ack_reqid) {
    const char *WHERE = "Sock_Stream::exec_abandon_ack";
    short       lv_fserr;
    int         lv_sockerr;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d, can-ack-reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid, pv_can_ack_reqid);
    lv_fserr = exec_com_chk(WHERE);
    if (gv_ms_trace_abandon)
        trace_where_printf(WHERE, "reqid=%d, can-ack-reqid=%d, fserr=%d\n",
                           pv_reqid, pv_can_ack_reqid, lv_fserr);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_ABANDON_ACK, pp_md, lv_fserr);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
        return lv_fserr;
    }

    exec_com_init_simple(pp_md, pv_reqid, pv_can_ack_reqid);
    lv_sockerr = send_md(MS_OP_ABANDON_ACK,
                         MS_PMH_TYPE_ABANDON_ACK,
                         MD_STATE_ABANDON_ACK_SENDING,
                         0,
                         pp_md,
                         XZFIL_ERR_OK);
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
    return lv_fserr;
}

short SB_Trans::Sock_Stream::exec_close(MS_Md_Type *pp_md,
                                        int         pv_reqid) {
    const char *WHERE = "Sock_Stream::exec_close";
    MS_SS_Type *lp_s = &pp_md->iv_ss;
    short       lv_fserr;
    int         lv_opts;
    int         lv_sockerr;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid);
    lv_fserr = exec_com_chk(WHERE);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_CLOSE, pp_md, lv_fserr);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
        return lv_fserr;
    }

    iv_req_map.put(pp_md);
    exec_com_init_simple(pp_md, pv_reqid, -1);
    lp_s->iv_req_ctrl_size = static_cast<int>(strlen(ga_ms_su_pname) + 1);
    lp_s->ip_req_ctrl = static_cast<short *>(MS_BUF_MGR_ALLOC(lp_s->iv_req_ctrl_size));
    // TODO: if can't get buffer
    pp_md->out.ip_recv_ctrl = reinterpret_cast<char *>(lp_s->ip_req_ctrl);
    strcpy(reinterpret_cast<char *>(lp_s->ip_req_ctrl), ga_ms_su_pname);
    if (iv_ic)
        lv_opts = MS_OPTS_MSIC;
    else
        lv_opts = 0;
    lv_sockerr = send_md(MS_OP_CLOSE,
                         MS_PMH_TYPE_CLOSE,
                         MD_STATE_CLOSE_SENDING,
                         lv_opts,
                         pp_md,
                         XZFIL_ERR_OK);
    if (lv_sockerr) {
        iv_req_map.remove(pp_md->iv_link.iv_id.i);
        lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    } else {
        exec_com_wait_send_done(WHERE, pp_md);
        lv_fserr = XZFIL_ERR_OK;
    }
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
    return lv_fserr;
}

short SB_Trans::Sock_Stream::exec_close_ack(MS_Md_Type *pp_md,
                                            int         pv_reqid) {
    const char *WHERE = "Sock_Stream::exec_close_ack";
    short       lv_fserr;
    int         lv_sockerr;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid);
    lv_fserr = exec_com_chk(WHERE);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_CLOSE_ACK, pp_md, lv_fserr);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
        return lv_fserr;
    }

    exec_com_init_simple(pp_md, pv_reqid, -1);
    lv_sockerr = send_md(MS_OP_CLOSE_ACK,
                         MS_PMH_TYPE_CLOSE_ACK,
                         MD_STATE_CLOSE_ACK_SENDING,
                         0,
                         pp_md,
                         XZFIL_ERR_OK);
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
    return lv_fserr;
}

//
// Purpose: execute chk
//
short SB_Trans::Sock_Stream::exec_com_chk(const char *pp_where) {
    short lv_fserr;

    if (gv_ms_trace)
        trace_where_printf(pp_where, "stream=%p %s, errored=%d\n",
                           pfp(this), ia_stream_name,
                           iv_sock_errored);
    if (iv_sock_errored) {
        if (gv_ms_trace)
            trace_where_printf(pp_where, "stream=%p %s, errored=%d, sock=%d\n",
                               pfp(this), ia_stream_name,
                               iv_sock_errored, iv_sock);
        lv_fserr = ms_err_sock_to_fserr(pp_where, iv_sock_errored);
        return lv_fserr;
    }
    return XZFIL_ERR_OK;
}

//
// Purpose: execute error
//
void SB_Trans::Sock_Stream::exec_com_error(MS_Md_Op_Type  pv_op,
                                           MS_Md_Type    *pp_md,
                                           short          pv_fserr) {
    pp_md->iv_op = pv_op;
    finish_send(pp_md->ip_where,
                pp_md->ip_where,
                pp_md,
                pv_fserr,
                true);
}

//
// Purpose: execute initialize-simple
//
void SB_Trans::Sock_Stream::exec_com_init_simple(MS_Md_Type *pp_md,
                                                 int         pv_reqid,
                                                 int         pv_pri) {
    MS_SS_Type *lp_s = &pp_md->iv_ss;

    lp_s->iv_src = 0;
    lp_s->iv_dest = 0;
    lp_s->iv_req_id = pv_reqid;
    lp_s->iv_pri = pv_pri;
    lp_s->ip_req_ctrl = NULL;
    lp_s->iv_req_ctrl_size = 0;
    lp_s->ip_req_data = NULL;
    lp_s->iv_req_data_size = 0;
    lp_s->iv_rep_max_ctrl_size = 0;
    lp_s->iv_rep_max_data_size = 0;
}

//
// Purpose: execute wait-send
//
void SB_Trans::Sock_Stream::exec_com_wait_send_done(const char *pp_where,
                                                    MS_Md_Type *pp_md) {
    const char *WHERE = "Sock_Stream::exec_com_wait_send_done";
    bool        lv_send_done;
    int         lv_status;

    lv_send_done = pp_md->iv_send_done;
    if (gv_ms_trace)
        trace_where_printf(WHERE, "%s ENTER stream=%p %s, wait send-done=%d, msgid=%d, md=%p\n",
                           pp_where, pfp(this), ia_stream_name, lv_send_done,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));
    while (!pp_md->iv_send_done) {
        comp_sends();
        lv_status = pp_md->iv_cv2.wait(true, 0, 100); // need lock
        if (lv_status == ETIMEDOUT)
            continue;
        SB_util_assert_ieq(lv_status, 0);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "stream=%p %s, wait NOT send-done, msgid=%d, md=%p\n",
                               pfp(this), ia_stream_name,
                               pp_md->iv_link.iv_id.i, pfp(pp_md));
    }
    if (gv_ms_trace)
        trace_where_printf(WHERE, "%s EXIT stream=%p %s, wait send-done, msgid=%d, md=%p\n",
                           pp_where, pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));
}

short SB_Trans::Sock_Stream::exec_conn_ack(MS_Md_Type * /*pp_md*/,
                                           int          /*pv_reqid*/) {
    SB_util_abort("NOT implemented"); // TODO: NOT implemented
    return XZFIL_ERR_OK;
}

short SB_Trans::Sock_Stream::exec_open(MS_Md_Type *pp_md,
                                       int         pv_reqid) {
    const char *WHERE = "Sock_Stream::exec_open";
    MS_SS_Type *lp_s = &pp_md->iv_ss;
    short       lv_fserr;
    short       lv_opts;
    int         lv_sockerr;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid);
    lv_fserr = exec_com_chk(WHERE);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_OPEN, pp_md, lv_fserr);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
        return lv_fserr;
    }

    iv_req_map.put(pp_md);
    exec_com_init_simple(pp_md, pv_reqid, -1);
    lp_s->iv_req_ctrl_size = static_cast<int>(strlen(ga_ms_su_pname) + 1);
    lp_s->ip_req_ctrl =
      static_cast<short *>(MS_BUF_MGR_ALLOC(lp_s->iv_req_ctrl_size));
    // TODO: if can't get buffer
    pp_md->out.ip_recv_ctrl = reinterpret_cast<char *>(lp_s->ip_req_ctrl);
    strcpy(reinterpret_cast<char *>(lp_s->ip_req_ctrl), ga_ms_su_pname);
    if (iv_ic)
        lv_opts = MS_OPTS_MSIC;
    else
        lv_opts = 0;
    lv_sockerr = send_md(MS_OP_OPEN,
                         MS_PMH_TYPE_OPEN,
                         MD_STATE_OPEN_SENDING,
                         lv_opts,
                         pp_md,
                         XZFIL_ERR_OK);
    if (lv_sockerr)
        iv_req_map.remove(pp_md->iv_link.iv_id.i);
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
    return lv_fserr;
}

short SB_Trans::Sock_Stream::exec_open_ack(MS_Md_Type *pp_md,
                                           int         pv_reqid) {
    const char *WHERE = "Sock_Stream::exec_open_ack";
    short       lv_fserr;
    int         lv_sockerr;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid);
    lv_fserr = exec_com_chk(WHERE);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_OPEN_ACK, pp_md, lv_fserr);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
        return lv_fserr;
    }

    exec_com_init_simple(pp_md, pv_reqid, -1);
    lv_sockerr = send_md(MS_OP_OPEN_ACK,
                         MS_PMH_TYPE_OPEN_ACK,
                         MD_STATE_OPEN_ACK_SENDING,
                         0,
                         pp_md,
                         XZFIL_ERR_OK);
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
    return lv_fserr;
}

short SB_Trans::Sock_Stream::exec_reply(MS_Md_Type *pp_md,
                                        int         pv_src,
                                        int         pv_dest,
                                        int         pv_reqid,
                                        short      *pp_req_ctrl,
                                        int         pv_req_ctrl_size,
                                        char       *pp_req_data,
                                        int         pv_req_data_size,
                                        short       pv_fserr) {
    const char *WHERE = "Sock_Stream::exec_reply";
    MS_SS_Type *lp_s = &pp_md->iv_ss;
    short       lv_fserr;
    int         lv_sockerr;
    int         lv_status;

    lv_fserr = exec_com_chk(WHERE);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_REPLY, pp_md, lv_fserr);
        return lv_fserr;
    }

    lv_status = pp_md->iv_abandon_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    if (pp_md->iv_abandoned)
        iv_ms_abandon_callback(pp_md, false); // remove from this list
    else
        iv_rep_map.put(pp_md);
    lv_status = pp_md->iv_abandon_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lp_s->iv_src = pv_src,
    lp_s->iv_dest = pv_dest,
    lp_s->iv_req_id = pv_reqid;
    lp_s->iv_pri = -1;
    lp_s->ip_req_ctrl = pp_req_ctrl;
    lp_s->iv_req_ctrl_size = pv_req_ctrl_size,
    lp_s->ip_req_data = pp_req_data;
    lp_s->iv_req_data_size = pv_req_data_size,
assert(lp_s->iv_req_data_size >= 0);
    lp_s->iv_rep_max_ctrl_size = 0;
    lp_s->iv_rep_max_data_size = 0;
    lv_sockerr = send_md(MS_OP_REPLY,
                         MS_PMH_TYPE_REPLY,
                         MD_STATE_REPLY_SENDING,
                         0,
                         pp_md,
                         pv_fserr);
    if (lv_sockerr && !pp_md->iv_abandoned)
        iv_rep_map.remove(pp_md->iv_link.iv_id.i);
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    return lv_fserr;
}

short SB_Trans::Sock_Stream::exec_reply_nack(MS_Md_Type *pp_md,
                                             int         pv_reqid) {
    const char *WHERE = "Sock_Stream::exec_reply_nack";
    short       lv_fserr;
    int         lv_sockerr;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid);
    lv_fserr = exec_com_chk(WHERE);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_REPLY_NACK, pp_md, lv_fserr);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
        return lv_fserr;
    }

    exec_com_init_simple(pp_md, pv_reqid, -1);
    lv_sockerr = send_md(MS_OP_REPLY_NACK,
                         MS_PMH_TYPE_REPLY_NACK,
                         MD_STATE_REPLY_NACK_SENDING,
                         0,
                         pp_md,
                         XZFIL_ERR_OK);
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT fserr=%d\n", lv_fserr);
    return lv_fserr;
}

short SB_Trans::Sock_Stream::exec_reply_nw(MS_Md_Type *pp_md,
                                           int         pv_src,
                                           int         pv_dest,
                                           int         pv_reqid,
                                           short      *pp_req_ctrl,
                                           int         pv_req_ctrl_size,
                                           char       *pp_req_data,
                                           int         pv_req_data_size) {
    const char *WHERE = "Sock_Stream::exec_reply_nw";
    MS_SS_Type *lp_s = &pp_md->iv_ss;
    short       lv_fserr;
    int         lv_sockerr;

    lv_fserr = exec_com_chk(WHERE);
    if (lv_fserr != XZFIL_ERR_OK) {
        exec_com_error(MS_OP_REPLY_NW, pp_md, lv_fserr);
        return lv_fserr;
    }

    if (pp_md->iv_abandoned) {
        // don't bother to send a reply
        if (gv_ms_trace) {
            SB_Buf_Where la_where;
            sprintf(la_where, "%s %s", WHERE, pp_md->ip_where);
            trace_where_printf(la_where, "stream=%p %s, msg abandoned, reqid=%d\n",
                               pfp(this), ia_stream_name, pv_reqid);
        }
        finish_send(pp_md->ip_where,
                    pp_md->ip_where,
                    pp_md,
                    XZFIL_ERR_OK,
                    true);
        return 0;
    }
    iv_rep_map.put(pp_md);
    lp_s->iv_src = pv_src,
    lp_s->iv_dest = pv_dest,
    lp_s->iv_req_id = pv_reqid;
    lp_s->iv_pri = -1;
    lp_s->ip_req_ctrl = pp_req_ctrl;
    lp_s->iv_req_ctrl_size = pv_req_ctrl_size,
    lp_s->ip_req_data = pp_req_data;
    lp_s->iv_req_data_size = pv_req_data_size,
assert(lp_s->iv_req_data_size >= 0);
    lp_s->iv_rep_max_ctrl_size = 0;
    lp_s->iv_rep_max_data_size = 0;
    lv_sockerr = send_md(MS_OP_REPLY_NW,
                         MS_PMH_TYPE_REPLY,
                         MD_STATE_REPLY_SENDING,
                         0,
                         pp_md,
                         XZFIL_ERR_OK);
    if (lv_sockerr)
        iv_rep_map.remove(pp_md->iv_link.iv_id.i);
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    return lv_fserr;
}

short SB_Trans::Sock_Stream::exec_wr(MS_Md_Type *pp_md,
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
    const char *WHERE = "Sock_Stream::exec_wr";
    MS_Md_Type *lp_md;
    MS_SS_Type *lp_s = &pp_md->iv_ss;
    short       lv_fserr;
    int         lv_opts;
    int         lv_sockerr;

    // need to fill out md in case there's an error
    lv_opts = 0;
    if (pv_opts) {
        if (pv_opts & MS_OPTS_FSREQ)
            lv_opts |= MS_PMH_OPT_FSREQ;
        if (pv_opts & MS_OPTS_MSIC)
            lv_opts |= MS_PMH_OPT_MSIC;
    }
    pp_md->out.iv_fserr = XZFIL_ERR_OK;
    pp_md->out.ip_reply_ctrl = pp_rep_ctrl;
    pp_md->out.iv_reply_ctrl_max = pv_rep_max_ctrl_size;
    pp_md->out.ip_reply_data = pp_rep_data;
    pp_md->out.iv_reply_data_max = pv_rep_max_data_size;
    pp_md->out.iv_nid = iv_opened_nid;
    pp_md->out.iv_pid = iv_opened_pid;
#ifdef SQ_PHANDLE_VERIFIER
    pp_md->out.iv_verif = iv_opened_verif;
#endif
    pp_md->out.iv_msg_type = 0;
    pp_md->out.iv_ptype = iv_opened_type;
    pp_md->out.iv_pri = pv_pri;
    lp_s->iv_src = pv_src;
    lp_s->iv_dest = pv_dest;
    lp_s->iv_req_id = pv_reqid;
    lp_s->iv_pri = pv_pri;
    lp_s->ip_req_ctrl = pp_req_ctrl;
    lp_s->iv_req_ctrl_size = pv_req_ctrl_size;
    lp_s->ip_req_data = pp_req_data;
    lp_s->iv_req_data_size = pv_req_data_size;
assert(lp_s->iv_req_data_size >= 0);
    lp_s->iv_rep_max_ctrl_size = pv_rep_max_ctrl_size;
    lp_s->iv_rep_max_data_size = pv_rep_max_data_size;

    lv_fserr = exec_com_chk(WHERE);
    if (lv_fserr != XZFIL_ERR_OK) {
        pp_md->iv_send_done = true; // need for md to free
        finish_reply(pp_md, lv_fserr, true, true);
        return lv_fserr;
    }

    iv_req_map.put(pp_md);
    lv_sockerr = send_md(MS_OP_WR,
                         MS_PMH_TYPE_WR,
                         MD_STATE_WR_SENDING,
                         lv_opts,
                         pp_md,
                         XZFIL_ERR_OK);
    if (lv_sockerr)
        lp_md = static_cast<MS_Md_Type *>(iv_req_map.remove(pp_md->iv_link.iv_id.i));
    else
        lp_md = NULL;
    lv_fserr = ms_err_sock_to_fserr(WHERE, lv_sockerr);
    if (lv_fserr != XZFIL_ERR_OK) {
        // reply could have already happened
        if (lp_md != NULL)
            finish_reply(pp_md, lv_fserr, true, true);
    }
    return lv_fserr;
}


//
// Purpose: get (first) ib ip-address
// Scan interfaces for IB
//
bool SB_Trans::Sock_Stream::get_ib_addr(void *pp_addr) {
    const char         *WHERE = "Sock_Stream::get_ib_addr";
    enum {              MAX_IFS = 100 };
    char                la_errno[100];
    struct ifreq        la_ifreqs[MAX_IFS];
    struct sockaddr_in *lp_addr;
    unsigned char      *lp_addr_str;
    struct ifreq       *lp_ifreq;
    int                 lv_errno;
    struct ifconf       lv_ifconf;
    struct ifreq        lv_ifreq_hw;
    struct ifreq        lv_ifreq_up;
    int                 lv_inx;
    unsigned int        lv_ioctl_req;
    int                 lv_max;
    bool                lv_ret;
    int                 lv_sock;

    lv_ret = false;
    lv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (lv_sock < 0) {
        lv_errno = errno;
        if (gv_ms_trace_sock)
            trace_where_printf(WHERE, "socket(AF_INET, SOCK_DGRAM, 0), errno=%d(%s)\n",
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        return lv_ret;
    }

    lv_ifconf.ifc_len = static_cast<int>(sizeof(*lp_ifreq) * MAX_IFS);
    lv_ifconf.ifc_req = la_ifreqs;
    lv_ioctl_req = SIOCGIFCONF;
    if (ioctl(lv_sock, lv_ioctl_req, &lv_ifconf) == -1) {
        lv_errno = errno;
        if (gv_ms_trace_sock)
            trace_where_printf(WHERE, "ioctl(SIOCGIFCONF), errno=%d(%s)\n",
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        close(lv_sock);
        return lv_ret;
    }

    lv_max = static_cast<int>(lv_ifconf.ifc_len / sizeof(struct ifreq));

    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "ifreq max=%d\n", lv_max);
    for (lv_inx = 0; lv_inx < lv_max; lv_inx++) {
        lp_ifreq = &la_ifreqs[lv_inx];
        if (gv_ms_trace_sock)
            trace_where_printf(WHERE, "ifreq[%d] .sa_family=%d, .ifr_name=%s\n",
                               lv_inx, lp_ifreq->ifr_addr.sa_family,
                               lp_ifreq->ifr_name);

        if (lp_ifreq->ifr_addr.sa_family != AF_INET) {
            if (gv_ms_trace_sock)
                trace_where_printf(WHERE, "ifreq[%d] NOT AF_INET\n", lv_inx);
            continue;
        }

        // check if is up
        lv_ioctl_req = SIOCGIFFLAGS;
        memcpy(lv_ifreq_up.ifr_name,
               lp_ifreq->ifr_name,
               sizeof(lv_ifreq_up.ifr_name));
        if (ioctl(lv_sock, lv_ioctl_req, &lv_ifreq_up) == -1) {
            lv_errno = errno;
            if (gv_ms_trace_sock)
                trace_where_printf(WHERE, "ifreq[%d] ioctl(SIOCGIFFLAGS), errno=%d(%s)\n",
                                   lv_inx,
                                   lv_errno,
                                   strerror_r(lv_errno,
                                              la_errno,
                                              sizeof(la_errno)));
            continue;
        }
        if ((lv_ifreq_up.ifr_flags & IFF_UP) == 0) {
            if (gv_ms_trace_sock)
                trace_where_printf(WHERE, "ifreq[%d] NOT IFF_UP\n", lv_inx);
            continue;
        }

        // check if is ib
        lv_ioctl_req = SIOCGIFHWADDR;
        memcpy(lv_ifreq_hw.ifr_name,
               lp_ifreq->ifr_name,
               sizeof(lv_ifreq_hw.ifr_name));
        if (ioctl(lv_sock, lv_ioctl_req, &lv_ifreq_hw) == -1) {
            lv_errno = errno;
            if (gv_ms_trace_sock)
                trace_where_printf(WHERE, "ifreq[%d] ioctl(SIOCGIFHWADDR), errno=%d(%s)\n",
                                   lv_inx,
                                   lv_errno,
                                   strerror_r(lv_errno,
                                              la_errno,
                                              sizeof(la_errno)));
            continue;
        }
        if (lv_ifreq_hw.ifr_hwaddr.sa_family != ARPHRD_INFINIBAND) {
            if (gv_ms_trace_sock)
                trace_where_printf(WHERE, "ifreq[%d] NOT ARPHRD_INFINIBAND\n", lv_inx);
            continue;
        }
        lp_addr = reinterpret_cast<struct sockaddr_in *>(&lp_ifreq->ifr_addr);
        memcpy(pp_addr, lp_addr, sizeof(*lp_addr));
        if (gv_ms_trace_sock) {
            lp_addr_str = reinterpret_cast<unsigned char *>(&lp_addr->sin_addr.s_addr);
            trace_where_printf(WHERE, "ifreq[%d] addr=%d.%d.%d.%d\n",
                               lv_inx,
                               lp_addr_str[0],
                               lp_addr_str[1],
                               lp_addr_str[2],
                               lp_addr_str[3]);
        }
        lv_ret = true;
        break;
    }
    close(lv_sock);
    return lv_ret;
}

//
// Purpose: open port (ib)
//
int SB_Trans::Sock_Stream::open_port(char *pp_port) {
    const char         *WHERE = "Sock_Stream::open_port";
    static char         la_host[MAX_HOST];
    static char        *lp_host;
    unsigned char      *lp_addr;
    struct sockaddr_in  lv_addr;
    static int          lv_err;
    static bool         lv_first = true;
    bool                lv_ok;
    static int          lv_port;
    struct hostent     *lp_hostent = NULL;

    if (lv_first) {
        lv_first = false;
        if (gv_ms_ic_ibv) {
            lv_ok = get_ib_addr(&lv_addr);
            SB_util_assert_it(lv_ok);
            lp_addr = reinterpret_cast<unsigned char *>(&lv_addr.sin_addr.s_addr);
            sprintf(la_host, "%d.%d.%d.%d",
                    lp_addr[0], lp_addr[1], lp_addr[2], lp_addr[3]);
        } else {
            lv_err = gethostname(la_host, sizeof(la_host));
            SB_util_assert_if(lv_err);
            lp_hostent = gethostbyname( la_host );
            SB_util_assert_if(!lp_hostent);
            lp_addr = reinterpret_cast<unsigned char *>(lp_hostent->h_addr);
            sprintf(la_host, "%d.%d.%d.%d",
                    lp_addr[0], lp_addr[1], lp_addr[2], lp_addr[3]);
            
        }
        lp_host = la_host;
        cp_listener = new Sock_Listener();
        cp_listener->set_sdp(gv_ms_ic_ibv);
        lv_port = 0;
        cp_listener->listen(lp_host, &lv_port);
    }
    sprintf(pp_port, "%s:%d", lp_host, lv_port);
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "return port=%s\n", pp_port);
    return lv_err;
}

//
// Purpose: process control-info
//
void SB_Trans::Sock_Stream::process_ctrl(MS_Md_Type *pp_md, bool *pp_cont) {
    MS_PMH_Type  *lp_hdr;
    RInfo_Type   *lp_r;
    Rd_Type      *lp_rd;
    int           lv_len;

    lp_r = &iv_r;        // ref iv_r through lp_r
    lp_rd = &lp_r->iv_rd;
    lp_hdr = &lp_rd->iv_hdr;

    lv_len = lp_hdr->iv_dsize;
    lp_rd->iv_data_len = lv_len;
    if (pp_md != NULL)
        pp_md->out.iv_reply_data_count = lv_len;
    if (lv_len) {
        if (pp_md != NULL) {
            lp_r->iv_rd.ip_data = static_cast<char *>(pp_md->out.ip_reply_data);
            SB_util_assert_ile(lv_len, pp_md->out.iv_reply_data_max); // sw fault
        } else {
            lp_r->iv_rd.ip_data = static_cast<char *>(MS_BUF_MGR_ALLOC(lv_len));
            // TODO: if can't get buffer
        }
        recv_buf_init(lp_r, lp_rd->ip_data, lv_len, pp_cont);
    } else {
        lp_r->iv_rd.ip_data = NULL;
        *pp_cont = true;
    }
}

//
// Purpose: process header
//
void SB_Trans::Sock_Stream::process_hdr(MS_Md_Type **ppp_md, bool *pp_cont) {
    const char  *WHERE = "Sock_Stream::process_hdr";
    MS_PMH_Type  *lp_hdr;
    MS_Md_Type   *lp_md;
    RInfo_Type   *lp_r;
    Rd_Type      *lp_rd;
    int           lv_len;

    lp_r = &iv_r;        // ref iv_r through lp_r
    lp_rd = &lp_r->iv_rd;
    lp_hdr = &lp_rd->iv_hdr;

#ifdef USE_SEND_LAT
    {
        struct timeval lv_stop;
        gettimeofday(&lv_stop, NULL);
        struct timeval *lp_start =
         reinterpret_cast<struct timeval *>(lp_r->iv_rd.iv_hdr.ia_ts);
        long lv_us = (lv_stop.tv_sec * SB_US_PER_SEC + lv_stop.tv_usec) -
                     (lp_start->tv_sec * SB_US_PER_SEC + lp_start->tv_usec);
        long lv_sec = lv_us / SB_US_PER_SEC;
        lv_us -= lv_sec * SB_US_PER_SEC;
        ga_stream_lat_elapsed[gv_stream_lat_inx].tv_sec = lv_sec;
        ga_stream_lat_elapsed[gv_stream_lat_inx].tv_usec = lv_us;
    }
    gv_stream_lat_inx++;
    if (gv_stream_lat_inx >= MAX_LAT)
        gv_stream_lat_inx = 0;
#endif

    if ((lp_rd->iv_hdr.ia_sig[0] != MS_PMH_SIG0) ||
        (lp_rd->iv_hdr.ia_sig[1] != MS_PMH_SIG1) ||
        (lp_rd->iv_hdr.ia_sig[2] != MS_PMH_SIG2) ||
        (lp_rd->iv_hdr.ia_sig[3] != MS_PMH_SIG3)) {
        if (gv_ms_trace) {
            trace_where_printf(lp_rd->ip_where,
                               "recv hdr %s sig MISMATCH sig=%d.%d.%d.%d\n",
                               ia_stream_name,
                               lp_rd->iv_hdr.ia_sig[0],
                               lp_rd->iv_hdr.ia_sig[1],
                               lp_rd->iv_hdr.ia_sig[2],
                               lp_rd->iv_hdr.ia_sig[3]);
        }
        SB_util_abort("invalid lp_rd->iv_hdr.ia_sig"); // sw fault
    }
    if (gv_ms_assert_chk) {
        int lv_chk = -1;
        int *lp_hdr_chk = reinterpret_cast<int *>(&lp_rd->iv_hdr);
        int lv_limit = static_cast<int>(offsetof(MS_PMH_Type, iv_chk_hdr))/4;
        for (int lv_inx = 0; lv_inx < lv_limit; lv_inx++)
            lv_chk += lp_hdr_chk[lv_inx];
        SB_util_assert_ieq(lv_chk, lp_rd->iv_hdr.iv_chk_hdr);
    }
    if (gv_ms_trace_sock) {
        char la_hdr_type[100];
        get_hdr_type(lp_hdr->iv_type, la_hdr_type);
        trace_where_printf(lp_rd->ip_where,
#ifdef SQ_PHANDLE_VERIFIER
                          "recv hdr %s majv=%d, minv=%d, type=%d(%s), ptype=%d, opts=%d/%d, fserr=%d, reqid=%d, pri=%d, tag=%d, seq=%d.%d, p-id=%d/%d/" PFVY ", csize=%d, dsize=%d, cmax=%d, dmax=%d\n",
#else
                          "recv hdr %s majv=%d, minv=%d, type=%d(%s), ptype=%d, opts=%d/%d, fserr=%d, reqid=%d, pri=%d, tag=%d, seq=%d.%d, p-id=%d/%d, csize=%d, dsize=%d, cmax=%d, dmax=%d\n",
#endif
                          ia_stream_name,
                          lp_hdr->iv_maj,
                          lp_hdr->iv_min,
                          lp_hdr->iv_type,
                          la_hdr_type,
                          lp_hdr->iv_ptype,
                          lp_hdr->iv_opts,
                          lp_hdr->iv_opts2,
                          lp_hdr->iv_fserr,
                          lp_hdr->iv_reqid,
                          lp_hdr->iv_pri,
                          lp_hdr->iv_tag,
                          lp_hdr->iv_seq1,
                          lp_hdr->iv_seq2,
                          iv_remote_nid,
                          iv_remote_pid,
#ifdef SQ_PHANDLE_VERIFIER
                          iv_remote_verif,
#endif
                          lp_hdr->iv_csize,
                          lp_hdr->iv_dsize,
                          lp_hdr->iv_cmaxsize,
                          lp_hdr->iv_dmaxsize);
    }
    lp_r->iv_section = RSECTION_CTRL;
    lv_len = lp_hdr->iv_csize;
    lp_rd->iv_ctrl_len = lv_len;
    if (!(gv_ms_buf_options & MS_BUF_OPTION_COPY)) {
        if (lp_hdr->iv_type == MS_PMH_TYPE_REPLY) {
            lp_md = Msg_Mgr::map_to_md(lp_r->iv_rd.iv_hdr.iv_reqid, WHERE);
            SB_util_assert_pne(lp_md, NULL); // sw fault
            lp_md->out.iv_reply_ctrl_count = lv_len;
        } else
            lp_md = NULL;
    } else
        lp_md = NULL;
    *ppp_md = lp_md;
    if (lv_len) {
        if (lp_md != NULL) {
            lp_r->iv_rd.ip_ctrl = reinterpret_cast<char *>(lp_md->out.ip_reply_ctrl);
            SB_util_assert_ile(lv_len, lp_md->out.iv_reply_ctrl_max); // sw fault
        } else {
            lp_r->iv_rd.ip_ctrl = static_cast<char *>(MS_BUF_MGR_ALLOC(lv_len));
            // TODO: if can't get buffer
        }
        recv_buf_init(lp_r, lp_rd->ip_ctrl, lv_len, pp_cont);
    } else {
        lp_r->iv_rd.ip_ctrl = NULL;
        *pp_cont = true;
    }
}

//
// Purpose: recv buf (continue)
//
void SB_Trans::Sock_Stream::recv_buf_cont(RInfo_Type *pp_r,
                                          bool       *pp_cont) {
    const char *WHERE = "Sock_Stream::recv_buf_cont";
    char        la_errno[100];
    size_t      lv_count;
    int         lv_errno;
    ssize_t     lv_rc;

    sock_lock();
    if (ip_sock != NULL) {
        lv_count = pp_r->iv_recv_size - pp_r->iv_recv_count;
        lv_rc = ip_sock->read(&pp_r->ip_buf[pp_r->iv_recv_count],
                              lv_count,
                              &lv_errno);
        if (lv_rc > 0) {
            pp_r->iv_recv_count += lv_rc;
            if (gv_ms_trace_sm)
                trace_where_printf(WHERE, "sock=%d, recv-count=" PFSZ ", recv-size=" PFSZ "\n",
                                   iv_sock,
                                   pp_r->iv_recv_count,
                                   pp_r->iv_recv_size);
            if (pp_r->iv_recv_count >= pp_r->iv_recv_size)
                pp_r->iv_state = RSTATE_RCVD;
            *pp_cont = true;
        } else if (lv_rc == 0) {
            iv_sock_errored = EHOSTDOWN;
            if (gv_ms_trace_sm)
                trace_where_printf(WHERE, "sock=%d, state=EOF, errno=%d(%s)\n",
                                   iv_sock,
                                   iv_sock_errored,
                                   strerror_r(iv_sock_errored,
                                              la_errno,
                                              sizeof(la_errno)));
            pp_r->iv_state = RSTATE_EOF;
        } else if (lv_errno == EWOULDBLOCK)
            pp_r->iv_state = RSTATE_RCVING;
        else {
            iv_sock_errored = lv_errno;
            if (gv_ms_trace_sm)
                trace_where_printf(WHERE,
                                   "sock=%d, state=ERROR, errno=%d(%s)\n",
                                   iv_sock,
                                   iv_sock_errored,
                                   strerror_r(iv_sock_errored,
                                              la_errno,
                                              sizeof(la_errno)));
            pp_r->iv_state = RSTATE_ERROR;
        }
    }
    sock_unlock();
}

//
// Purpose: recv buf (initial)
//
void SB_Trans::Sock_Stream::recv_buf_init(RInfo_Type *pp_r,
                                          void       *pp_buf,
                                          size_t      pv_count,
                                          bool       *pp_cont) {
    const char *WHERE = "Sock_Stream::recv_buf_init";
    int         lv_errno;
    ssize_t     lv_rc;

    sock_lock();
    if (ip_sock != NULL) {
        pp_r->ip_buf = static_cast<char *>(pp_buf);
        pp_r->iv_recv_size = pv_count;

        lv_rc = ip_sock->read(pp_buf, pv_count, &lv_errno);
        if (lv_rc > 0) {
            pp_r->iv_recv_count = lv_rc;
            if (gv_ms_trace_sm)
                trace_where_printf(WHERE, "sock=%d, recv-count=" PFSZ ", recv-size=" PFSZ "\n",
                                   iv_sock,
                                   pp_r->iv_recv_count,
                                   pp_r->iv_recv_size);
            if (lv_rc >= pp_r->iv_recv_size)
                pp_r->iv_state = RSTATE_RCVD;
            else
                pp_r->iv_state = RSTATE_RCVING;
            *pp_cont = true;
        } else {
            pp_r->iv_recv_count = 0;
            if (lv_rc == 0) {
                pp_r->iv_state = RSTATE_EOF;
                iv_sock_errored = EHOSTDOWN;
                *pp_cont = true;
            } else if (lv_errno == EWOULDBLOCK)
                pp_r->iv_state = RSTATE_RCVING;
            else {
                pp_r->iv_state = RSTATE_ERROR;
                iv_sock_errored = lv_errno;
                *pp_cont = true;
            }
        }
    }
    sock_unlock();
}

//
// Purpose: send abandon-ack
//
void SB_Trans::Sock_Stream::send_abandon_ack(MS_Md_Type *pp_md,
                                             int         pv_reqid,
                                             int         pv_can_ack_reqid) {
    const char  *WHERE = "Sock_Stream::send_abandon_ack";
    Stream_Base *lp_stream;

    // delegate to helper thread - comp-thread can't do this work
    pp_md->iv_op = MS_OP_ABANDON_ACK;
    pp_md->iv_aa_reqid = pv_reqid;
    pp_md->iv_aa_can_ack_reqid = pv_can_ack_reqid;
    if (gv_ms_trace || gv_ms_trace_abandon)
        trace_where_printf(WHERE, "stream=%p %s, add-to-helper, msgid=%d, md=%p, ms.op=%d\n",
                           pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pp_md->iv_op);
    if (cp_helper_thread != NULL && !gv_ms_shutdown_called)
        cp_helper_thread->add(pp_md);
    else {
        // helper can't do this, so do it directly
        if (gv_ms_trace)
            trace_where_printf(WHERE, "sending abandon-ack DIRECT\n");
        lp_stream = static_cast<Stream_Base *>(pp_md->ip_stream);
        lp_stream->exec_abandon_ack(pp_md,
                                    pp_md->iv_aa_reqid,
                                    pp_md->iv_aa_can_ack_reqid);
    }
}

//
// Purpose: send buf (continue)
//
void SB_Trans::Sock_Stream::send_buf_cont(MS_SS_Type *pp_s,
                                          bool       *pp_cont) {
    const char *WHERE = "Sock_Stream::send_buf_cont";
    size_t      lv_count;
    int         lv_errno;
    ssize_t     lv_wc;

    sock_lock();
    if (ip_sock != NULL) {
        lv_count = pp_s->iv_send_size - pp_s->iv_send_count;
        lv_wc = ip_sock->write(&pp_s->ip_buf[pp_s->iv_send_count],
                               lv_count,
                               &lv_errno);
        if (lv_wc > 0) {
            pp_s->iv_send_count += static_cast<int>(lv_wc);
            if (gv_ms_trace_sm)
                trace_where_printf(WHERE, "sock=%d, send-count=%d, send-size=%d\n",
                                   iv_sock,
                                   pp_s->iv_send_count,
                                   pp_s->iv_send_size);
            if (pp_s->iv_send_count >= pp_s->iv_send_size)
                pp_s->iv_state = SSTATE_SENT;
            *pp_cont = true;
        } else if (lv_errno == EWOULDBLOCK) {
            pp_s->iv_state = SSTATE_SENDING;
                // turn on EPOLLOUT
            ip_sock->event_change(EPOLLIN | EPOLLOUT, ip_sock_eh);
        } else {
            pp_s->iv_state = SSTATE_ERROR;
            iv_sock_errored = lv_errno;
        }
    }
    sock_unlock();
}

//
// Purpose: send buf (initial)
//
void SB_Trans::Sock_Stream::send_buf_init(MS_SS_Type *pp_s,
                                          void       *pp_buf,
                                          size_t      pv_count,
                                          bool       *pp_cont) {
    const char *WHERE = "Sock_Stream::send_buf_init";
    int         lv_errno;
    ssize_t     lv_wc;

    sock_lock();
    if (ip_sock != NULL) {
        pp_s->ip_buf = static_cast<char *>(pp_buf);
        pp_s->iv_send_size = static_cast<int>(pv_count);

        lv_wc = ip_sock->write(pp_buf, pv_count, &lv_errno);
        if (lv_wc > 0) {
            pp_s->iv_send_count = static_cast<int>(lv_wc);
            if (gv_ms_trace_sm)
                trace_where_printf(WHERE, "sock=%d, send-count=%d, send-size=%d\n",
                                   iv_sock,
                                   pp_s->iv_send_count,
                                   pp_s->iv_send_size);
            if (lv_wc >= pp_s->iv_send_size)
                pp_s->iv_state = SSTATE_SENT;
            else
                pp_s->iv_state = SSTATE_SENDING;
            *pp_cont = true;
        } else {
            pp_s->iv_send_count = 0;
            if (lv_errno == EWOULDBLOCK)
                pp_s->iv_state = SSTATE_SENDING;
            else {
                pp_s->iv_state = SSTATE_ERROR;
                iv_sock_errored = lv_errno;
                *pp_cont = true;
            }
        }
    }
    sock_unlock();
}

//
// Purpose: send md
//
int SB_Trans::Sock_Stream::send_md(MS_Md_Op_Type  pv_op,
                                   int            pv_hdr_type,
                                   int            pv_state,
                                   int            pv_opts,
                                   MS_Md_Type    *pp_md,
                                   short          pv_fserr) {
    const char   *WHERE = "Sock_Stream::send_md";
    SB_Buf_Where  la_where;
    MS_Md_Type   *lp_md;
    MS_SS_Type   *lp_s = &pp_md->iv_ss;
    bool          lv_abandoned;
    int           lv_err;
    int           lv_inx;
    bool          lv_reply;
    bool          lv_req_ctrl;
    int           lv_req_ctrl_size;
    bool          lv_req_data;
#ifdef HDR_DATA
    int           lv_req_data_size;
#endif
    int           lv_status;

    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_SEND_MD,
                       pp_md->iv_link.iv_id.i,
                       pv_op);
    if (gv_ms_trace)
        sprintf(la_where, "%s %s", WHERE, pp_md->ip_where);

    pp_md->iv_md_state = pv_state; // set before setting op (for process_abandon)
    if (pv_op != MS_OP_REPLY)
        pp_md->iv_op = pv_op; // need to set under send-mutex
    lv_reply = (pv_op == MS_OP_REPLY) || (pv_op == MS_OP_REPLY_NACK);

    // now check for abandon
    if (lv_reply && pp_md->iv_abandoned) {
        lv_status = pp_md->iv_abandon_mutex.lock();
        pp_md->iv_op = pv_op; // ok to set here
        SB_util_assert_ieq(lv_status, 0); // sw fault
        if (gv_ms_trace || gv_ms_trace_abandon)
            trace_where_printf(WHERE, "stream=%p %s, do not send abandoned reply msgid=%d, md=%p\n",
                               pfp(this),
                               ia_stream_name,
                               pp_md->iv_msgid,
                               pfp(pp_md));
        finish_send(pp_md->ip_where,
                    pp_md->ip_where,
                    pp_md,
                    XZFIL_ERR_OK,
                    true);
        lv_status = pp_md->iv_abandon_mutex.unlock();
        SB_util_assert_ieq(lv_status, 0); // sw fault
        return 0;
    }

    lp_s->iv_req_count = 0; // set before putting in map
    lp_s->iv_hdr_size = MS_PMH_HDR_SIZE;
    lp_s->iv_hdr.ia_sig[0] = MS_PMH_SIG0;
    lp_s->iv_hdr.ia_sig[1] = MS_PMH_SIG1;
    lp_s->iv_hdr.ia_sig[2] = MS_PMH_SIG2;
    lp_s->iv_hdr.ia_sig[3] = MS_PMH_SIG3;
    lp_s->iv_hdr.iv_maj = MS_PMH_MAJOR;
    lp_s->iv_hdr.iv_min = MS_PMH_MINOR;
    lp_s->iv_hdr.iv_order = MS_PMH_ORDER_LE;
    lp_s->iv_hdr.iv_type = static_cast<char>(pv_hdr_type);
    lp_s->iv_hdr.iv_ptype = static_cast<char>(gv_ms_su_ptype);
    lp_s->iv_hdr.iv_opts = static_cast<char>(pv_opts);
    lp_s->iv_hdr.iv_opts2 = 0;
    lp_s->iv_hdr.iv_rsvd = 0;
    lp_s->iv_hdr.iv_fserr = pv_fserr;
    lp_s->iv_hdr.iv_reqid = lp_s->iv_req_id;
    lp_s->iv_hdr.iv_pri = lp_s->iv_pri;
    lp_s->iv_hdr.iv_tag = lp_s->iv_tag_hdr;
    lp_s->iv_hdr.iv_seq1 = lp_s->iv_src;
    lp_s->iv_hdr.iv_seq2 = iv_seq++;
    lp_s->iv_hdr.iv_csize = lp_s->iv_req_ctrl_size;
    lp_s->iv_hdr.iv_dsize = lp_s->iv_req_data_size;
    lp_s->iv_hdr.iv_cmaxsize = lp_s->iv_rep_max_ctrl_size;
    lp_s->iv_hdr.iv_dmaxsize = lp_s->iv_rep_max_data_size;
#ifdef USE_SEND_LAT
    gettimeofday(reinterpret_cast<timeval *>(lp_s->iv_hdr.ia_ts), NULL);
#endif
    if (gv_ms_assert_chk) { // send_md (generate checksum)
        int lv_chk = -1;
        int *lp_hdr = reinterpret_cast<int *>(&lp_s->iv_hdr);
        int lv_limit = static_cast<int>(offsetof(MS_PMH_Type, iv_chk_hdr))/4;
        for (lv_inx = 0; lv_inx < lv_limit; lv_inx++)
            lv_chk += lp_hdr[lv_inx];
        lp_s->iv_hdr.iv_chk_hdr = lv_chk;
        lv_chk = -1;
        char *lp_data = reinterpret_cast<char *>(lp_s->ip_req_ctrl);
        lv_limit = lp_s->iv_req_ctrl_size;
        for (lv_inx = 0; lv_inx < lv_limit; lv_inx++)
            lv_chk += lp_data[lv_inx];
        lp_s->iv_hdr.iv_chk_ctrl = lv_chk;
        lv_chk = -1;
        lp_data = lp_s->ip_req_data;
        lv_limit = lp_s->iv_req_data_size;
        for (lv_inx = 0; lv_inx < lv_limit; lv_inx++)
            lv_chk += lp_data[lv_inx];
        lp_s->iv_hdr.iv_chk_data = lv_chk;
    }
    if (lp_s->iv_req_ctrl_size > 0)
        lv_req_ctrl_size = lp_s->iv_req_ctrl_size;
    else
        lv_req_ctrl_size = 0;
#ifdef HDR_CTRL
    if (lv_req_ctrl_size <= MS_PMH_CTRL_DATA_LEN) {
        lp_s->iv_hdr_size += lv_req_ctrl_size;
        memcpy(lp_s->iv_hdr.ia_ctrl, lp_s->ip_req_ctrl, lv_req_ctrl_size);
    }
#endif
#ifdef HDR_DATA
    if (lv_req_ctrl_size <= MS_PMH_CTRL_DATA_LEN) {
        lp_s->iv_hdr_size += lv_req_ctrl_size;
        memcpy(lp_s->iv_hdr.ia_data, lp_s->ip_req_ctrl, lv_req_ctrl_size);
    }
    if (lp_s->iv_req_data_size > 0)
        lv_req_data_size = lp_s->iv_req_data_size;
    else
        lv_req_data_size = 0;
    if (lv_req_ctrl_size + lv_req_data_size <= MS_PMH_CTRL_DATA_LEN) {
        lp_s->iv_hdr_size += lv_req_data_size;
        memcpy(&lp_s->iv_hdr.ia_data[lv_req_ctrl_size],
               lp_s->ip_req_data,
               lv_req_data_size);
    }
#endif

    if (lv_reply)
        lp_s->iv_req_count = 1;
    //
    // need to set iv_send_done here so comp thread can manipulate
    // invalidate slots so comp thread won't get confused
    //
    lp_s->iv_send_done = 0;
    if (lp_s->iv_req_ctrl_size > MS_PMH_CTRL_DATA_LEN)
        lv_req_ctrl = true;
    else {
        lv_req_ctrl = false;
        lp_s->iv_send_done |= SEND_DONE_CTRL;
        lp_s->iv_slot_ctrl = -1;
    }
    if ((lp_s->iv_req_data_size > 0) &&
        ((MS_PMH_CTRL_LEN > 0) ||
         ((MS_PMH_DATA_LEN > 0) &&
          (lp_s->iv_req_ctrl_size + lp_s->iv_req_data_size > MS_PMH_CTRL_DATA_LEN))))
        lv_req_data = true;
    else {
        lv_req_data = false;
        lp_s->iv_send_done |= SEND_DONE_DATA;
        lp_s->iv_slot_data = -1;
    }

    if (gv_ms_trace_sock) {
        char la_hdr_type[100];
        char la_where[100];
        get_hdr_type(lp_s->iv_hdr.iv_type, la_hdr_type);
        sprintf(la_where, "%s %s", WHERE, pp_md->ip_where);
        trace_where_printf(la_where,
#ifdef SQ_PHANDLE_VERIFIER
                           "send hdr %s msgid=%d, md=%p majv=%d, minv=%d, type=%d(%s), ptype=%d, opts=%d/%d, fserr=%d, reqid=%d, pri=%d, tag=%d, seq=%d.%d, p-id=%d/%d/" PFVY ", csize=%d, dsize=%d, cmax=%d, dmax=%d\n",
#else
                           "send hdr %s msgid=%d, md=%p majv=%d, minv=%d, type=%d(%s), ptype=%d, opts=%d/%d, fserr=%d, reqid=%d, pri=%d, tag=%d, seq=%d.%d, p-id=%d/%d, csize=%d, dsize=%d, cmax=%d, dmax=%d\n",
#endif
                           ia_stream_name,
                           pp_md->iv_msgid,
                           pfp(pp_md),
                           lp_s->iv_hdr.iv_maj,
                           lp_s->iv_hdr.iv_min,
                           lp_s->iv_hdr.iv_type,
                           la_hdr_type,
                           lp_s->iv_hdr.iv_ptype,
                           lp_s->iv_hdr.iv_opts,
                           lp_s->iv_hdr.iv_opts2,
                           lp_s->iv_hdr.iv_fserr,
                           lp_s->iv_hdr.iv_reqid,
                           lp_s->iv_hdr.iv_pri,
                           lp_s->iv_hdr.iv_tag,
                           lp_s->iv_hdr.iv_seq1,
                           lp_s->iv_hdr.iv_seq2,
                           iv_remote_nid,
                           iv_remote_pid,
#ifdef SQ_PHANDLE_VERIFIER
                           iv_remote_verif,
#endif
                           lp_s->iv_hdr.iv_csize,
                           lp_s->iv_hdr.iv_dsize,
                           lp_s->iv_hdr.iv_cmaxsize,
                           lp_s->iv_hdr.iv_dmaxsize);
    }
    if (gv_ms_trace_sock) {
        char la_where[100];
        sprintf(la_where, "%s %s", WHERE, pp_md->ip_where);
        trace_where_printf(la_where, "send header\n");
    }

    if (lv_req_ctrl) {
        if (lv_reply)
            lp_s->iv_req_count++;

        if (gv_ms_trace_sock) {
            char la_where[100];
            sprintf(la_where, "%s %s", WHERE, pp_md->ip_where);
            trace_where_printf(la_where, "send ctrl %s, len=%d\n",
                               pp_md->ip_where,
                               lp_s->iv_req_ctrl_size);
        }
    }

    if (lv_req_data) {
        if (lv_reply)
            lp_s->iv_req_count++;

        if (gv_ms_trace_sock) {
            char la_where[100];
            sprintf(la_where, "%s %s", WHERE, pp_md->ip_where);
            trace_where_printf(la_where, "send data %s, len=%d\n",
                               pp_md->ip_where,
                               lp_s->iv_req_data_size);
        }
    }

    // for a given socket, these sends have to be *grouped*
    lv_status = iv_send_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    if (pv_op == MS_OP_REPLY)
        pp_md->iv_op = pv_op; // set it now!
    if (lv_reply && pp_md->iv_abandoned) {
        // forget about doing reply
        lv_err = 0;
        lv_abandoned = true;
    } else if (iv_sock_errored) {
        lv_abandoned = false;
        lv_err = iv_sock_errored;
    } else {
        // if thread was stuck due to a lock
        lv_abandoned = false;
        lv_err = 0;
        lp_s->iv_state = SSTATE_INIT;
        pp_md->iv_tid = SB_Thread::Sthr::self_id();
        ip_send_md = pp_md;
        send_sm();
        if (lp_s->iv_state < SSTATE_DONE) {
            lv_status = pp_md->iv_cv.wait(true);
            SB_util_assert_ieq(lv_status, 0);
            pp_md->iv_cv.reset_flag();
        }
    }
    lv_abandoned = lv_abandoned; // touch
    lv_status = iv_send_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);

    if (lv_reply) {
        pp_md->iv_md_state = MD_STATE_REPLY_SEND_FIN; // done with REPLY
        for (;;) {
            lv_status = iv_reply_piggyback_mutex.lock();
            SB_util_assert_ieq(lv_status, 0); // sw fault
            lp_md = remove_reply_piggyback();
            lv_status = iv_reply_piggyback_mutex.unlock();
            SB_util_assert_ieq(lv_status, 0); // sw fault
            if (lp_md == NULL)
                break;
            exec_abandon_ack(lp_md,
                             lp_md->iv_aa_reqid,
                             lp_md->iv_aa_can_ack_reqid);
        }
        finish_send(pp_md->ip_where,
                    pp_md->ip_where,
                    pp_md,
                    XZFIL_ERR_OK,
                    true);
    } else {
        finish_send(pp_md->ip_where,
                    pp_md->ip_where,
                    pp_md,
                    XZFIL_ERR_OK,
                    true);
    }
    return lv_err;
}

//
// Purpose: send-sm
//
void SB_Trans::Sock_Stream::send_sm() {
    static const char *la_section[] = {
        "HDR",
        "CTRL",
        "DATA"
    };
    static const char *la_state[] = {
        "INIT",
        "SEND",
        "SENDING",
        "SENT",
        "DONE",
        "ERROR",
        "EOF"
    };
    const char   *WHERE = "Sock_Stream::send_sm";
    MS_Md_Type   *lp_md = ip_send_md;
    MS_SS_Type   *lp_s = &lp_md->iv_ss;
    const char   *lp_section = NULL; // make compiler happy
    const char   *lp_state = NULL; // make compiler happy
    bool          lv_cont;

    lv_cont = true;
    while (lv_cont) {
        lv_cont = false;

        if (gv_ms_trace_sm) {
            if ((lp_s->iv_state >= SSTATE_INIT) &&
                (lp_s->iv_state < SSTATE_LAST))
                lp_state = la_state[lp_s->iv_state];
            else
                lp_state = "<unknown>";
            if ((lp_s->iv_section >= SSECTION_HDR) &&
                (lp_s->iv_section <= SSECTION_DATA))
                lp_section = la_section[lp_s->iv_section];
            else
                lp_section = "<unknown>";
            trace_where_printf(WHERE, "sock=%d, state=%d(%s), section=%d(%s)\n",
                               iv_sock,
                               lp_s->iv_state, lp_state,
                               lp_s->iv_section, lp_section);
        }

        switch (lp_s->iv_state) {
        case SSTATE_INIT:
            lp_s->iv_state = SSTATE_SEND;
            lp_s->iv_section = SSECTION_HDR;
            lv_cont = true;
            break;

        case SSTATE_SEND:
            send_buf_init(lp_s,
                          &lp_s->iv_hdr,
                          lp_s->iv_hdr_size,
                          &lv_cont);
            break;

        case SSTATE_SENDING:
            send_buf_cont(lp_s, &lv_cont);
            break;

        case SSTATE_SENT:
            switch (lp_s->iv_section) {
            case SSECTION_HDR:
                lp_s->iv_section = SSECTION_CTRL;
                if ((lp_s->iv_send_done & SEND_DONE_CTRL) == 0)
                    send_buf_init(lp_s,
                                  lp_s->ip_req_ctrl,                // buf
                                  lp_s->iv_req_ctrl_size,           // count
                                  &lv_cont);
                else
                    lv_cont = true;
                break;

            case SSECTION_CTRL:
                lp_s->iv_section = SSECTION_DATA;
                if ((lp_s->iv_send_done & SEND_DONE_DATA) == 0)
                    send_buf_init(lp_s,
                                  lp_s->ip_req_data,                // buf
                                  lp_s->iv_req_data_size,           // count
                                  &lv_cont);
                else
                    lv_cont = true;
                break;

            case SSECTION_DATA:
                lp_s->iv_state = SSTATE_DONE;
                lv_cont = true;
                break;
            }
            break;

        case SSTATE_DONE:
            // turn off EPOLLOUT
            sock_lock();
            if (ip_sock != NULL) {
                ip_sock->event_change(EPOLLIN, ip_sock_eh);
            }
            sock_unlock();
            if (lp_md->iv_tid != SB_Thread::Sthr::self_id())
                lp_md->iv_cv.signal(true);
            break;

        case SSTATE_ERROR:
            finish_writereads(ms_err_sock_to_fserr(WHERE,
                              iv_sock_errored));
            stop_sock();
            break;
        }
    }
}

//
// (static)
//
void SB_Trans::Sock_Stream::shutdown() {
    close_streams(false);
}

void SB_Trans::Sock_Stream::sock_free() {
    const char *WHERE = "Sock_Stream::sock_free";

    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "ENTER\n");
    sock_lock();
    if (ip_sock != NULL) {
        ip_sock->destroy();
        ip_sock = NULL;
    }
    sock_unlock();
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "EXIT\n");
}

void SB_Trans::Sock_Stream::sock_lock() {
    int lv_status;

    lv_status = iv_sock_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
}

void SB_Trans::Sock_Stream::sock_unlock() {
    int lv_status;

    lv_status = iv_sock_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: start stream
//
int SB_Trans::Sock_Stream::start_stream() {
    const char *WHERE = "Sock_Stream::start_stream";
    char        la_name[10];

    if (cp_accept_thread == NULL) {
        strcpy(la_name, "accept");
        cp_accept_thread =
          new Sock_Stream_Accept_Thread(la_name,
                                        ga_ms_su_sock_a_port,
                                        this,
                                        &msg_mon_accept_sock_cbt);
        if (gv_ms_trace_sock)
            trace_where_printf(WHERE, "starting sock accept thread %s\n",
                               la_name);
        cp_accept_thread->start();

        strcpy(la_name, "helper");
        cp_helper_thread = new Sock_Stream_Helper_Thread(la_name);
        if (gv_ms_trace)
            trace_where_printf(WHERE, "starting helper thread %s\n", la_name);
        cp_helper_thread->start();
    }
    return 0;
}

//
// Purpose: stop recv
//
void SB_Trans::Sock_Stream::stop_recv() {
    const char *WHERE = "Sock_Stream::stop_recv";

    if (get_thread_marker()) {
        if (gv_ms_trace)
            trace_where_printf(WHERE, "stream=%p %s, comp thread ENTER\n",
                               pfp(this), ia_stream_name);
        if (!iv_close_ind_sent) {
            iv_close_ind_sent  = true;
            send_close_ind();
        }
        stop_sock();
        if (gv_ms_trace)
            trace_where_printf(WHERE, "stream=%p %s, comp thread EXIT\n",
                               pfp(this), ia_stream_name);
        return;
    }
}

//
// Purpose: stop sock
//
void SB_Trans::Sock_Stream::stop_sock() {
    sock_lock();
    if (ip_sock != NULL)
        ip_sock->stop();
    sock_unlock();
}

//
// Purpose: stop completions
//
void SB_Trans::Sock_Stream::stop_completions() {
}

//
// Purpose: start accept thread
//
static void *sock_stream_accept_thread_fun(void *pp_arg) {
    SB_Trans::Sock_Stream_Accept_Thread *lp_thread =
      static_cast<SB_Trans::Sock_Stream_Accept_Thread *>(pp_arg);
    lp_thread->run();
    return NULL;
}

//
// Purpose: start helper thread
//
static void *sock_helper_thread_fun(void *pp_arg) {
    SB_Trans::Sock_Stream_Helper_Thread *lp_thread =
      static_cast<SB_Trans::Sock_Stream_Helper_Thread *>(pp_arg);
    lp_thread->run();
    return NULL;
}

//
// Purpose: constructor accept thread
//
SB_Trans::Sock_Stream_Accept_Thread::Sock_Stream_Accept_Thread(const char                 *pp_name,
                                                               const char                 *pp_port,
                                                               Sock_Stream                *pp_sock_stream,
                                                               SB_Mon_Sock_Accept_Cb_Type  pv_cb)
: Thread(sock_stream_accept_thread_fun, pp_name),
  ip_port(pp_port),
  ip_sock_stream(pp_sock_stream),
  iv_cb(pv_cb),
  iv_fin(false),
  iv_running(false) {
    const char *WHERE = "Sock_Stream_Accept_Thread::Sock_Stream_Accept_Thread";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "creating this=%p\n", pfp(this));
}

//
// Purpose: destructor accept thread
//
SB_Trans::Sock_Stream_Accept_Thread::~Sock_Stream_Accept_Thread() {
    const char *WHERE = "Sock_Stream_Accept_Thread::~Sock_Stream_Accept_Thread";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "destroying this=%p\n", pfp(this));
}

//
// Purpose: finish accept thread
//
void SB_Trans::Sock_Stream_Accept_Thread::fin() {
    char         la_host[SB_Trans::Sock_Stream::MAX_HOST];
    Sock_Client  lv_client;
    int          lv_port;

    iv_fin = true;
    ip_sock_stream->cp_listener->get_addr(la_host, &lv_port);
    lv_client.set_sdp(gv_ms_ic_ibv);
    lv_client.connect(la_host, lv_port);
}

//
// Purpose: run accept thread
//
void SB_Trans::Sock_Stream_Accept_Thread::run() {
    const char *WHERE = "Sock_Stream_Accept_Thread::run";

    sb_trans_sock_stream_setup_signals();
    iv_running = true;
    while (!iv_fin) {
        Sock_Server *lp_server = ip_sock_stream->cp_listener->accept();
        if (lp_server != NULL) {
            if (iv_fin)
                delete lp_server;
            else
                iv_cb(lp_server);
        } else
            iv_fin = true;
    }
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "EXITING accept thread\n");
    ip_sock_stream->cp_listener->destroy();
    ip_sock_stream->cp_listener = NULL;
    iv_running = false;
}

//
// Purpose: accept thread running?
//
bool SB_Trans::Sock_Stream_Accept_Thread::running() {
    return iv_running;
}

//
// Purpose: constructor socket stream event handler
//
SB_Trans::Sock_Stream_EH::Sock_Stream_EH(Sock_Stream *pp_stream)
: ip_stream(pp_stream) {
    const char *WHERE = "Sock_Stream_EH::Sock_Stream_EH";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "creating this=%p\n", pfp(this));
}

//
// Purpose: destructor socket stream event handler
//
SB_Trans::Sock_Stream_EH::~Sock_Stream_EH() {
    const char *WHERE = "Sock_Stream_EH::~Sock_Stream_EH";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "destroying this=%p\n", pfp(this));
}

//
// Purpose: lock eh
//
void SB_Trans::Sock_Stream_EH::eh_lock() {
    int lv_status;

    lv_status = iv_eh_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

//
// Purpose: unlock eh
//
void SB_Trans::Sock_Stream_EH::eh_unlock() {
    int lv_status;

    lv_status = iv_eh_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

//
// Purpose: socket stream event handler process events
//
void SB_Trans::Sock_Stream_EH::process_events(int pv_events) {
    eh_lock();
    if (ip_stream != NULL) {
        ip_stream->process_events(pv_events);
    }
    eh_unlock();
}

//
// Purpose: set stream
//
void SB_Trans::Sock_Stream_EH::set_stream(Sock_Stream *pp_stream) {
    ip_stream = pp_stream;
}

//
// Purpose: constructor socket stream helper
//
SB_Trans::Sock_Stream_Helper_Thread::Sock_Stream_Helper_Thread(const char *pp_name)
: Thread(sock_helper_thread_fun, pp_name),
  iv_fin(false),
  iv_q("sock-stream-helper", false),
  iv_running(false) {
}

//
// Purpose: destructor socket stream helper
//
SB_Trans::Sock_Stream_Helper_Thread::~Sock_Stream_Helper_Thread() {
}

//
// Purpose: socket stream helper add md
//
void SB_Trans::Sock_Stream_Helper_Thread::add(MS_Md_Type *pp_md) {
    iv_q.add(reinterpret_cast<SB_QL_Type *>(&pp_md->iv_link));
}

//
// Purpose: socket stream helper finish
//
void SB_Trans::Sock_Stream_Helper_Thread::fin() {
    iv_fin = true;
    iv_md.iv_op = MS_OP_FLUSH; // fin signal
    iv_q.add(reinterpret_cast<SB_QL_Type *>(&iv_md.iv_link));
}

//
// Purpose: socket stream helper run
//
void SB_Trans::Sock_Stream_Helper_Thread::run() {
    const char  *WHERE = "Sock_Stream_Helper_Thread::run";
    MS_Md_Type  *lp_md;
    Stream_Base *lp_stream;

    sb_trans_sock_stream_setup_signals();
    iv_running = true;
    while (!iv_fin) {
        lp_md = static_cast<MS_Md_Type *>(iv_q.remove());
        switch (lp_md->iv_op) {
        case MS_OP_ABANDON_ACK:
            if (gv_ms_trace)
                trace_where_printf(WHERE, "exec msgid=%d, md=%p\n",
                                   lp_md->iv_link.iv_id.i, pfp(lp_md));
            lp_stream = static_cast<Stream_Base *>(lp_md->ip_stream);
            lp_stream->exec_abandon_ack(lp_md,
                                        lp_md->iv_aa_reqid,
                                        lp_md->iv_aa_can_ack_reqid);
            break;

        case MS_OP_FLUSH:
            break;

        case MS_OP_WR:
            if (gv_ms_trace)
                trace_where_printf(WHERE, "exec msgid=%d, md=%p\n",
                                   lp_md->iv_link.iv_id.i,
                                   pfp(lp_md));
#ifdef REWORK_MD
            Stream_Base *lp_stream = static_cast<Stream_Base *>(lp_md->ip_stream);
            lp_stream->exec(lp_md);
#endif

        default:
            SB_util_abort("invalid lp_md->iv_op"); // sw fault (invalid op)
            break;
        }
    }
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXITING helper thread\n");
    iv_running = false;
}

//
// Purpose: socket stream helper running?
//
bool SB_Trans::Sock_Stream_Helper_Thread::running() {
    return iv_running;
}
