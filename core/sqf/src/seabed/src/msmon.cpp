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
// msg_mon utilities
//
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>

#include "monclio.h"

#include "common/evl_sqlog_eventnum.h"

#include "seabed/fserr.h"
#include "seabed/labels.h"
#include "seabed/labelmaps.h"
#include "seabed/log.h"
#include "seabed/ms.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "apictr.h"
#include "auto.h"
#include "buf.h"
#include "cap.h"
#include "chk.h"
#include "env.h"
#include "fstrace.h"
#include "fsx.h"
#include "llmap.h"
#include "looptrans.h"
#include "mserrmsg.h"
#include "msic.h"
#include "mslog.h"
#include "msmon.h"
#include "msod.h"
#include "mstrace.h"
#include "msutil.h"
#include "msx.h"
#include "pctlx.h"
#include "phan.h"
#include "sbconst.h"
#include "smap.h"
#include "socktrans.h"
#include "tablemgr.h"
#include "threadtlsx.h"
#include "timermap.h"
#include "trans.h"
#include "util.h"
#include "utilalloc.h"
#include "utracex.h"

typedef void (*Fs_Shutdown)();
typedef union {
    Fs_Shutdown  ishutdown;
    void        *ipshutdown;
} Fs_Shutdown_Type;
typedef void (*Trace_Change)(const char *, const char *);
typedef union {
    Trace_Change  ichange;
    void         *ipchange;
} Trace_Change_Type;


//
// holds nid/pid (key) and state
//
typedef struct Ms_NPS_Node {
    SB_LLML_Type     iv_link;
    int              iv_state;
} Ms_NPS_Node;

//
// holds nid/pid
//
typedef struct Ms_NidPid_Type {
    union {
        struct {
            int iv_nid;
            int iv_pid;
        } i;
        SB_Int64_Type iv_nidpid;
    } u;
} Ms_NidPid_Type;

//
// Negotiation data
//
typedef struct Ms_Neg_Type {
    bool          iv_ic;
    int           iv_nid;
    int           iv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type iv_verif;
#endif
    char          ia_pname[MS_MON_MAX_PROCESS_NAME+1];
    char          ia_prog[SB_MAX_PROG];
    int           iv_fserr;
} Ms_Neg_Type;

//
// holds tag (key) and info
//
typedef struct Ms_TC_Node {
    SB_LLML_Type                 iv_link;
    MS_Mon_Start_Process_Cb_Type iv_callback;
} Ms_TC_Node;

//
// open thread.
//
class Ms_Open_Thread : public SB_Thread::Thread {
public:
    Ms_Open_Thread(const char                  *ip_where,
                   const char                  *pp_name,
                   MS_Mon_Open_Process_Cb_Type  pv_cb,
                   long long                    pv_tag,
                   bool                         pv_ic,
                   Ms_Od_Type                  *pp_od);
    virtual ~Ms_Open_Thread();

    void        fin();
    void        run();
    void        run_del();
    bool        running();
    static void shutdown();

private:
    void run_add();

    static SB_Ts_Lmap            cv_run_map;
    Ms_Od_Type                  *ip_od;
    const char                  *ip_where;
    MS_Mon_Open_Process_Cb_Type  iv_cb;
    MS_Mon_Open_Comp_Type        iv_comp;
    bool                         iv_fin;
    bool                         iv_ic;
    SB_LML_Type                  iv_run_link;
    bool                         iv_running;
};

class Ms_Tod {
public:
    Ms_Tod();
    ~Ms_Tod();

private:
    struct timeval iv_tod;
    struct tm      iv_tm;
};

// forwards
void        ms_od_cleanup_key_dtor(void *pp_map);
void        ms_od_cleanup_remove(int pv_oid);

// globals
enum                        { MS_MAX_MON_OUT   = 50 };
enum                        { MS_MAX_OPEN_OUT  = 50 };
enum                        { MS_MAX_TRACE_CBS = 10 };
char                          ga_ms_su_a_port[MPI_MAX_PORT_NAME]; // accept
char                          ga_ms_su_c_port[MPI_MAX_PORT_NAME]; // connect
static SB_Phandle_Type        gv_ms_su_phandle;
char                          ga_ms_su_prog[SB_MAX_PROG];
char                          ga_ms_su_sock_a_port[MPI_MAX_PORT_NAME]; // accept
                              // add 30 for nid/pid in pname
static char                   ga_ms_su_trace_pname[MS_MON_MAX_PROCESS_NAME+30];
static MS_Mon_Trace_Cb_Type   ga_ms_trace_callback[MS_MAX_TRACE_CBS];
static SB_Ts_LLmap            gv_ms_accept_map("map-ms-accept");
static SB_Thread::ECM         gv_ms_close_process_mutex("mutex-gv_ms_close_process");
#ifdef SQ_PHANDLE_VERIFIER
static SB_Ts_NPVmap           gv_ms_conn_state_map("map-ms-conn-state");
#else
static SB_Ts_LLmap            gv_ms_conn_state_map("map-ms-conn-state");
#endif
static SB_TimerMap            gv_ms_idle_timer_map("map-ms-idle-timer", 50);
static int                    gv_ms_max_openers = 0;
static Ms_Od_Table_Entry_Mgr  gv_ms_od_table_entry_mgr;
       Ms_Od_Table_Mgr        gv_ms_od_mgr("tablemgr-OD",
                                           SB_Table_Mgr_Alloc::ALLOC_FAST,
                                           SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                           &gv_ms_od_table_entry_mgr,
                                           10, 10); // cap-init, cap-inc
static SB_Smap                gv_ms_od_name_map("map-ms-od-name");
static SB_Thread::ECM         gv_ms_od_name_map_mutex("mutex-gv_ms_od_name_map_mutex");
static int                    gv_ms_od_tls_inx =
                                SB_create_tls_key(ms_od_cleanup_key_dtor,
                                                  "ms-od-cleanup");
static SB_Thread::Usem        gv_ms_open_sem;
static SB_Ts_Slot_Mgr         gv_ms_recv_tag_mgr("slotmgr-msmon-outstanding", SB_Slot_Mgr::ALLOC_FAST, MS_MAX_MON_OUT);
int                           gv_ms_streams = 0;
int                           gv_ms_streams_max = 0;
bool                          gv_ms_su_altsig           = false;
bool                          gv_ms_su_called           = false;
bool                          gv_ms_su_eventmsgs        = true;
int                           gv_ms_su_pnid             = -1;
int                           gv_ms_su_nid              = -1;
pthread_t                     gv_ms_su_pthread_self     = 0;
int                           gv_ms_su_pid              = -1;
bool                          gv_ms_su_pipeio           = true;
int                           gv_ms_su_ptype            = ProcessType_Undefined;
MS_Md_Type                   *gp_ms_rsvd_md             = NULL;
SB_Trans::Sock_Stream        *gp_ms_su_sock_stream      = NULL;
bool                          gv_ms_su_sysmsgs          = false;
Ms_Tod                        gv_ms_su_tod;
#ifdef SQ_PHANDLE_VERIFIER
SB_Verif_Type                 gv_ms_su_verif            = -1;
bool                          gv_ms_su_verif_used       = true;
#else
bool                          gv_ms_su_verif_used       = false;
#endif
static SB_Imap                gv_ms_tag_map("map-ms-tag");
static SB_Ts_LLmap            gv_ms_tc_map("map-ms-tc");
static int                    gv_ms_trace_callback_inx = 0;
static MS_Mon_Tmlib_Cb_Type   gv_ms_tmlib_callback      = NULL;
static MS_Mon_Tmlib2_Cb_Type  gv_ms_tmlib2_callback     = NULL;
SB_Ts_Lmap                    Ms_Open_Thread::cv_run_map("map-ms-open-thread-run");

typedef struct Map_Tag_Entry_Type {
    SB_ML_Type       iv_link;
} Map_Tag_Entry_Type;

typedef struct Map_Od_Entry_Type {
    SB_ML_Type  iv_link;
    Ms_Od_Type *ip_od;
} Map_Od_Entry_Type;

//
// forwards
//
static bool           msg_mon_accept_sock_callback_create(SB_Trans::Sock_User *pp_sock,
                                                          int                  pv_nid,
                                                          int                  pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                          SB_Verif_Type        pv_verif,
#endif
                                                          const char          *pp_pname,
                                                          const char          *pp_prog,
                                                          bool                 pv_ic);
static int            msg_mon_accept_sock_negotiate_id_srv(SB_Trans::Sock_User *pp_sock,
                                                           int                 *pp_nid,
                                                           int                 *pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                           SB_Verif_Type       *pp_verif,
#endif
                                                           char                *pp_pname,
                                                           char                *pp_prog,
                                                           bool                *pp_ic);
static int            msg_mon_close_process_od(Ms_Od_Type *pp_od,
                                               bool        pv_free_oid,
                                               bool        pv_send_mon);
static bool           msg_mon_conn_state_change(int           pv_nid,
                                                int           pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                SB_Verif_Type pv_verif,
#endif
                                                MSGTYPE       pv_msg_type);
static bool           msg_mon_msg_ok(const char   *pp_where,
                                     const char   *pp_where_detail,
                                     int          *pp_mpierr,
                                     Mon_Msg_Type *pp_msg,
                                     int           pv_msg_type,
                                     int           pv_reply_type);
static void           msg_mon_oc_cbt(MS_Md_Type *pp_md, void *pp_stream);
static int            msg_mon_open_process_com(char            *pp_name,
                                               SB_Phandle_Type *pp_phandle,
                                               int             *pp_oid,
                                               bool             pv_reopen,
                                               bool             pv_death_notif,
                                               bool             pv_self,
                                               bool             pv_backup,
                                               bool             pv_ic,
                                               bool             pv_fs_open);
static int            msg_mon_open_process_com_ph1(char             *pp_name,
                                                   SB_Phandle_Type  *pp_phandle,
                                                   int              *pp_oid,
                                                   bool              pv_reopen,
                                                   bool              pv_death_notif,
                                                   bool              pv_backup,
                                                   bool              pv_fs_open,
                                                   Ms_Od_Type      **ppp_od,
                                                   int              *pp_done);
static int            msg_mon_open_process_com_ph2(char            *pp_name,
                                                   SB_Phandle_Type *pp_phandle,
                                                   int             *pp_oid,
                                                   bool             pv_reopen,
                                                   bool             pv_death_notif,
                                                   bool             pv_ic,
                                                   Ms_Od_Type      *pp_od);
static int            msg_mon_open_process_com_ph2_sock(const char      *pp_where,
                                                        char            *pp_name,
                                                        SB_Phandle_Type *pp_phandle,
                                                        int             *pp_oid,
                                                        bool             pv_reopen,
                                                        bool             pv_ic,
                                                        Ms_Od_Type      *pp_od,
                                                        Mon_Msg_Type    *pp_msg,
                                                        int              pv_nid,
                                                        int              pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                        SB_Verif_Type    pv_verif,
#endif
                                                        int              pv_ptype);
static int            msg_mon_open_self(char            *pp_name,
                                        SB_Phandle_Type *pp_phandle,
                                        int             *pp_oid,
                                        bool             pv_ic);
static int            msg_mon_process_startup_com(bool pv_sysmsgs,
                                                  bool pv_attach,
                                                  bool pv_eventmsgs,
                                                  bool pv_pipeio,
                                                  bool pv_altsig,
                                                  bool pv_stderr_remap=true)
SB_THROWS_FATAL;
static int            msg_mon_process_startup_ph1(bool pv_attach, 
                                                  bool pv_altsig, 
                                                  bool pv_stderr_remap=true)
SB_THROWS_FATAL;
static void           msg_mon_recv_msg_cbt(MS_Md_Type *pp_md);
static void           msg_mon_recv_msg_cbt_discard(const char *pp_where,
                                                   MS_Md_Type *pp_md,
                                                   bool        pv_ic);
static void           msg_mon_recv_msg_change(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_msg_close(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_msg_close_or_death(int            pv_nid,
                                                      int            pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                      SB_Verif_Type  pv_verif,
#endif
                                                      char          *pp_process_name,
                                                      int            pv_aborted,
                                                      bool           pv_close);
static void           msg_mon_recv_msg_loc_cbt(Mon_Msg_Type *pp_msg,
                                               int           pv_size);
static void           msg_mon_recv_msg_node_down(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_msg_node_quiesce(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_msg_node_up(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_msg_process_created(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_msg_process_death(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_msg_shutdown(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_msg_unknown(Mon_Msg_Type *pp_msg);
static void           msg_mon_recv_notice_msg_loc_cbt(Mon_Msg_Type *pp_msg,
                                                      int           pv_size);
static int            msg_mon_send_node_info(const char   *pp_where,
                                             Mon_Msg_Type *pp_msg,
                                             int           pv_msg_err,
                                             int           pv_nid,
                                             bool          pv_cont);
static int            msg_mon_send_notify(const char         *pp_where,
                                          int                 pv_notify_nid,
                                          int                 pv_notify_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                          SB_Verif_Type       pv_notify_verif,
#endif
                                          bool                pv_cancel,
                                          int                 pv_target_nid,
                                          int                 pv_target_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                          SB_Verif_Type       pv_target_verif,
#endif
                                          MS_Mon_Transid_Type pv_target_transid,
                                          bool                pv_assert);
#ifdef SQ_PHANDLE_VERIFIER
static int            msg_mon_send_notify_verif(const char         *pp_where,
                                                const char         *pp_notify_name,
                                                bool                pv_cancel,
                                                const char         *pp_target_name,
                                                MS_Mon_Transid_Type pv_target_transid,
                                                bool                pv_assert);
#endif
static int            msg_mon_send_process_info(const char   *pp_where,
                                                Mon_Msg_Type *pp_msg,
                                                int           pv_msg_err,
                                                int           pv_nid,
                                                int           pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                SB_Verif_Type  pv_verif,
#endif
                                                char         *pp_name,
                                                int           pv_ptype,
                                                bool          pv_cont);
static int            msg_mon_send_zone_info(const char   *pp_where,
                                             Mon_Msg_Type *pp_msg,
                                             int           pv_msg_err,
                                             int           pv_nid,
                                             int           pv_zid,
                                             bool          pv_cont);
static int            msg_mon_sendrecv_mon(const char   *pp_where,
                                           const char   *pp_desc,
                                           Mon_Msg_Type *pp_msg,
                                           int           pv_msg_err);
static int            msg_mon_sendrecv_mon_lio(const char   *pp_where,
                                               const char   *pp_desc,
                                               Mon_Msg_Type *pp_msg);
static int            msg_mon_start_process_ph1(MS_Mon_Start_Process_Cb_Type  pv_callback,
                                                bool                          pv_nowait,
                                                char                         *pp_path,
                                                char                         *pp_ldpath,
                                                char                         *pp_prog,
                                                char                         *pp_name,
                                                char                         *pp_ret_name,
                                                int                           pv_argc,
                                                char                        **ppp_argv,
                                                SB_Phandle_Type              *pp_phandle,
                                                int                           pv_ptype,
                                                int                           pv_priority,
                                                int                           pv_debug,
                                                int                           pv_backup,
                                                long long                     pv_tag,
                                                int                          *pp_nid,
                                                int                          *pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                SB_Verif_Type                *pp_verif,
#endif
                                                char                         *pp_infile,
                                                char                         *pp_outfile,
                                                int                           pv_unhooked);
static void           msg_mon_trace_msg(const char   *pp_where,
                                        Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_change(const char   *pp_where,
                                               Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_close(const char   *pp_where,
                                              Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_node_down(const char   *pp_where,
                                                  Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_node_quiesce(const char   *pp_where,
                                                     Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_node_up(const char   *pp_where,
                                                Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_open(const char   *pp_where,
                                             Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_process_created(const char   *pp_where,
                                                        Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_process_death(const char   *pp_where,
                                                      Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_shutdown(const char   *pp_where,
                                                 Mon_Msg_Type *pp_msg);
static void           msg_mon_trace_msg_unknown(const char   *pp_where,
                                                Mon_Msg_Type *pp_msg);
static void           ms_fs_shutdown_ph1();
static void           ms_fs_shutdown_ph2();
static Ms_Od_Type    *ms_od_alloc();
void                  ms_od_cleanup_add(int pv_oid);
static void           ms_od_free(Ms_Od_Type *pp_od,
                                 bool        pv_free_oid,
                                 bool        pv_local,
                                 bool        pv_chkref);
static void           ms_od_map_name_add(char *pp_name, Ms_Od_Type *pp_od);
static void           ms_od_map_name_remove(char *pp_name);
static Ms_Od_Type    *ms_od_map_name_to_od(char *pp_name);
static Ms_Od_Type    *ms_od_map_oid_to_od(int pv_oid);
static Ms_Od_Type    *ms_od_map_phandle_to_od(SB_Phandle_Type *pp_phandle);
static int            ms_od_ref_add(Ms_Od_Type *pp_od);
static int            ms_od_ref_dec(Ms_Od_Type *pp_od);
static void           ms_stats(const char *pp_value, bool pv_trace);
static void           ms_stats_print(bool pv_trace, char *pp_line);
static void           ms_util_fill_phandle_oid(SB_Phandle_Type *pp_phandle,
                                               int              pv_oid);
static int            ms_util_map_ptype_to_compid(int pv_ms_su_ptype);
#ifdef SQ_PHANDLE_VERIFIER
static void           ms_util_name_seq(const char *pp_name_seq,
                                       char       *pp_name,
                                       size_t      pv_name_len,
                                       Verifier_t *pp_verif);
#endif
static void           ms_util_string_clear(char   *pp_dest,
                                           size_t  pv_dest_size);
static void           ms_util_string_copy(char       *pp_dest,
                                          size_t      pv_dest_size,
                                          const char *pp_src);

#ifdef SQ_PHANDLE_VERIFIER
static Ms_NPS_Node *new_NPS_Node(SB_NPV_Type pv_npv) {
#else
static Ms_NPS_Node *new_NPS_Node(SB_Int64_Type pv_nidpid) {
#endif
    Ms_NPS_Node *lp_node = new Ms_NPS_Node();
#ifdef SQ_PHANDLE_VERIFIER
    lp_node->iv_link.iv_id.npv = pv_npv;
#else
    lp_node->iv_link.iv_id.ll = pv_nidpid;
#endif
    return lp_node;
}

static Ms_TC_Node *new_TC_Node(long long                     pv_tag,
                                MS_Mon_Start_Process_Cb_Type pv_callback) {
    Ms_TC_Node *lp_node = new Ms_TC_Node();
    lp_node->iv_link.iv_id.ll = pv_tag;
    lp_node->iv_callback = pv_callback;
    return lp_node;
}

static void *ms_open_thread_fun(void *pp_arg) {
    Ms_Open_Thread *lp_thread = static_cast<Ms_Open_Thread *>(pp_arg);
    int             lv_status;

    lp_thread->run();
    lp_thread->delete_exit(false);
    lp_thread->run_del();
    delete lp_thread;
    lv_status = gv_ms_open_sem.post();
    SB_util_assert_ieq(lv_status, 0);
    return NULL;
}

Ms_Open_Thread::Ms_Open_Thread(const char                  *pp_where,
                               const char                  *pp_name,
                               MS_Mon_Open_Process_Cb_Type  pv_cb,
                               long long                    pv_tag,
                               bool                         pv_ic,
                               Ms_Od_Type                  *pp_od)
: SB_Thread::Thread(ms_open_thread_fun, pp_name),
  ip_od(pp_od),
  ip_where(pp_where),
  iv_cb(pv_cb),
  iv_fin(false),
  iv_ic(pv_ic),
  iv_running(false) {
    iv_comp.name = get_name();
    iv_comp.tag = pv_tag;
}

Ms_Open_Thread::~Ms_Open_Thread() {
}

void Ms_Open_Thread::fin() {
    iv_fin = true;
}

void Ms_Open_Thread::run() {
    const char *WHERE = "Ms_Open_Thread::run";

    iv_running = true;
    iv_comp.ferr = msg_mon_open_process_com_ph2(const_cast<char *>(iv_comp.name),
                                                &iv_comp.phandle,
                                                &iv_comp.oid,
                                                false, // reopen
                                                true,  // death notification
                                                iv_ic,
                                                ip_od);
    if (gv_ms_trace_mon) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, &iv_comp.phandle);
        trace_where_printf(ip_where, "calling completion callback, fserr=%d, pname=%s, oid=%d, phandle=%s, tag=0x%llx\n",
                           iv_comp.ferr,
                           iv_comp.name,
                           iv_comp.oid,
                           la_phandle,
                           iv_comp.tag);
    }
    iv_cb(&iv_comp);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXITING open thread\n");
    iv_running = false;
}

void Ms_Open_Thread::run_add() {
    iv_run_link.iv_id.l = reinterpret_cast<long>(this);
    cv_run_map.put(&iv_run_link);
}

void Ms_Open_Thread::run_del() {
    long lv_id = reinterpret_cast<long>(this);
    cv_run_map.remove(lv_id);
}

bool Ms_Open_Thread::running() {
    return iv_running;
}

void Ms_Open_Thread::shutdown() {
    SB_Lmap_Enum   *lp_enum;
    void           *lp_result;
    Ms_Open_Thread *lp_thread;
    long            lv_id;
    int             lv_status;

    while (!cv_run_map.empty()) {
        lp_enum = cv_run_map.keys();
        if (lp_enum->more()) {
            lv_id = lp_enum->next()->iv_id.l;
            lp_thread = reinterpret_cast<Ms_Open_Thread *>(lv_id);
            lv_status = lp_thread->join(&lp_result);
            SB_util_assert_ieq(lv_status, 0);
        }
        delete lp_enum;
    }
}

Ms_Tod::Ms_Tod() {
    gettimeofday(&iv_tod, NULL);
    localtime_r(&iv_tod.tv_sec, &iv_tm);
}

Ms_Tod::~Ms_Tod() {
}

//
// Purpose: Deal with accept
//
bool msg_mon_accept_sock_cbt(void *pp_sock) {
    char                 la_pname[MS_MON_MAX_PROCESS_NAME+1];
    char                 la_prog[SB_MAX_PROG];
    SB_Trans::Sock_User *lp_sock;
    bool                 lv_ic;
    int                  lv_nid;
    int                  lv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type        lv_verif;
#endif

    lp_sock = static_cast<SB_Trans::Sock_User *>(pp_sock);
    msg_mon_accept_sock_negotiate_id_srv(lp_sock,
                                         &lv_nid,
                                         &lv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         &lv_verif,
#endif
                                         la_pname,
                                         la_prog,
                                         &lv_ic);
    msg_mon_accept_sock_callback_create(lp_sock,
                                        lv_nid,
                                        lv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                        lv_verif,
#endif
                                        la_pname,
                                        la_prog,
                                        lv_ic);
    return true;
}

bool msg_mon_accept_sock_callback_create(SB_Trans::Sock_User  *pp_sock,
                                         int                   pv_nid,
                                         int                   pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         SB_Verif_Type         pv_verif,
#endif
                                         const char           *pp_pname,
                                         const char           *pp_prog,
                                         bool                  pv_ic) {
    const char   *WHERE = "msg_mon_accept_sock_callback_create";
    MS_Md_Type   *lp_md;
    Mon_Msg_Type *lp_msg;
    bool          lv_ret;
    int           lv_size;

    lv_ret = true;
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE,
                           "creating stream for sock=%d, p-id=%d/%d/" PFVY ", pname=%s, prog=%s, ic=%d\n",
                           pp_sock->get_sock(),
                           pv_nid,
                           pv_pid,
                           pv_verif,
                           pp_pname,
                           pp_prog,
                           pv_ic);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE,
                           "creating stream for sock=%d, p-id=%d/%d, pname=%s, prog=%s, ic=%d\n",
                           pp_sock->get_sock(),
                           pv_nid,
                           pv_pid,
                           pp_pname,
                           pp_prog,
                           pv_ic);
#endif
    SB_Buf_Line la_name;
    sprintf(la_name, "accept p-id=%d/%d (%s-%s)",
            pv_nid,
            pv_pid,
            pp_pname,
            pp_prog);
    if (pv_ic)
        strcat(la_name, "-IC");
    SB_Trans::Sock_Stream *lp_stream =
      SB_Trans::Sock_Stream::create(la_name,
                                    pp_pname,
                                    pp_prog,
                                    pv_ic,
                                    pp_sock,
                                    NULL,
                                    NULL,
                                    &ms_ldone_cbt,
                                    &ms_abandon_cbt,
                                    &msg_mon_oc_cbt,
                                    gv_ms_lim_cb,
                                    &gv_ms_recv_q,
                                    &gv_ms_lim_q,
                                    &gv_ms_event_mgr,
                                    pv_nid,
                                    pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                    pv_verif,
#endif
                                    -1,  // opened nid
                                    -1,  // opened pid
#ifdef SQ_PHANDLE_VERIFIER
                                    -1,  // opened verif
#endif
                                    -1); // opened ptype
    int lv_fserr = lp_stream->start_stream();
    if (lv_fserr != XZFIL_ERR_OK) {
        // backout
        SB_Trans::Trans_Stream::close_stream(WHERE,
                                             lp_stream,
                                             true,
                                             true,
                                             false);

        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXIT FAILURE to start stream, ret=%d\n",
                               lv_fserr);
        return false; // coverity find
    }

    //
    // Create monitor open msg
    //
    SB_Trans::Msg_Mgr::get_md(&lp_md, // msg_mon_accept_sock_callback_create
                              lp_stream,
                              NULL,
                              false,  // recv
                              NULL,   // fserr
                              WHERE,
                              MD_STATE_RCVD_MON_OPEN);
    SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
    lv_size = static_cast<int>(offsetof(Mon_Msg_Type, u.request.u.open) +
                               sizeof(lp_msg->u.request.u.open));
    lp_msg = static_cast<Mon_Msg_Type *>(MS_BUF_MGR_ALLOC(lv_size));
    // TODO: if can't get buffer
    lp_msg->type = MsgType_Open;
    lp_msg->u.request.type = ReqType_Open;
    lp_msg->u.request.u.open.nid = pv_nid;
    lp_msg->u.request.u.open.pid = pv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.open.verifier = pv_verif;
#endif
    ms_util_string_copy(lp_msg->u.request.u.open.target_process_name,
                        sizeof(lp_msg->u.request.u.open.target_process_name),
                        pp_pname);
    lp_msg->u.request.u.open.death_notification = true;
    lp_md->iv_tag = -1;
    lp_md->out.iv_mon_msg = true;
    lp_md->out.iv_ldone = false;
    lp_md->out.iv_nid = gv_ms_su_nid;
    lp_md->out.iv_pid = gv_ms_su_pid;
    lp_md->out.iv_recv_req_id = 0;
    lp_md->out.iv_pri = -1;
    lp_md->out.iv_recv_mpi_source_rank = -1;
    lp_md->out.iv_recv_mpi_tag = -1;
    lp_md->out.ip_recv_ctrl = NULL;
    lp_md->out.iv_recv_ctrl_size = 0;
    lp_md->out.ip_recv_data = reinterpret_cast<char *>(lp_msg);
    lp_md->out.iv_recv_data_size = lv_size;
    lp_md->out.iv_recv_ctrl_max = 0;
    lp_md->out.iv_recv_data_max = 0;
    if (gv_ms_trace_mon) {
        const char *lp_msg_type = msg_util_get_msg_type(lp_msg->type);
        trace_where_printf(WHERE, "manufacturing/queueing %s (%d) mon message\n",
                           lp_msg_type, lp_msg->type);
        msg_mon_trace_msg(WHERE, lp_msg);
    }
    msg_mon_conn_state_change(lp_msg->u.request.u.open.nid,
                              lp_msg->u.request.u.open.pid,
#ifdef SQ_PHANDLE_VERIFIER
                              lp_msg->u.request.u.open.verifier,
#endif
                              lp_msg->type);
    if (pv_ic)
        msg_mon_recv_msg_cbt_discard(WHERE, lp_md, true);
    else if (gv_ms_enable_messages) {
        if ((gv_ms_lim_cb != NULL) &&
            gv_ms_lim_cb(0, // mon-msg
                         NULL,
                         0,
                         lp_md->out.ip_recv_data,
                         lp_md->out.iv_recv_data_size)) {
            gv_ms_lim_q.add(&lp_md->iv_link);
            gv_ms_event_mgr.set_event_all(INTR);
        } else {
            gv_ms_recv_q.add(&lp_md->iv_link);
            gv_ms_event_mgr.set_event_all(LREQ);
        }
    } else
        msg_mon_recv_msg_cbt_discard(WHERE, lp_md, false);
    return lv_ret;
}

//
// Purpose: negotiate (client-side)
//
int msg_mon_accept_sock_negotiate_id_cli(SB_Trans::Sock_User *pp_sock,
                                         int                  pv_nid,
                                         int                  pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         SB_Verif_Type        pv_verif,
#endif
                                         bool                 pv_ic,
                                         char                *pp_prog) {
    const char  *WHERE = "msg_mon_accept_sock_negotiate_id_cli";
    int          lv_fserr;
    ssize_t      lv_rcnt;
    Ms_Neg_Type  lv_req;
    Ms_Neg_Type  lv_rsp;
    ssize_t      lv_wcnt;

    lv_fserr = XZFIL_ERR_OK;
    lv_req.iv_nid = gv_ms_su_nid;
    lv_req.iv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lv_req.iv_verif = gv_ms_su_verif;
#endif
    lv_req.iv_ic = pv_ic;
    lv_req.iv_fserr = XZFIL_ERR_OK;
    strcpy(lv_req.ia_pname, ga_ms_su_pname);
    strcpy(lv_req.ia_prog, ga_ms_su_prog);
    lv_wcnt = pp_sock->write(&lv_req, sizeof(lv_req), NULL);
    if (lv_wcnt != sizeof(lv_req))
        lv_fserr = XZFIL_ERR_PATHDOWN;
    if (lv_fserr == XZFIL_ERR_OK) {
        lv_rcnt = pp_sock->read(&lv_rsp, sizeof(lv_rsp), NULL);
        if (lv_rcnt != sizeof(lv_rsp))
            lv_fserr = XZFIL_ERR_PATHDOWN;
    }
    if (lv_fserr == XZFIL_ERR_OK) {
        pp_prog[SB_MAX_PROG-1] = '\0';
        strncpy(pp_prog, lv_rsp.ia_prog, SB_MAX_PROG-1);
        if (gv_ms_trace_mon)
#ifdef SQ_PHANDLE_VERIFIER
            trace_where_printf(WHERE, "server rsp p-id=%d/%d/" PFVY " (%s), prog=%s, err=%d\n",
#else
            trace_where_printf(WHERE, "server rsp p-id=%d/%d (%s), prog=%s, err=%d\n",
#endif
                               lv_rsp.iv_nid,
                               lv_rsp.iv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                               lv_rsp.iv_verif,
#endif
                               lv_rsp.ia_pname,
                               lv_rsp.ia_prog,
                               lv_rsp.iv_fserr);
        SB_util_assert_ieq(lv_rsp.iv_nid, pv_nid); // sw fault
        SB_util_assert_ieq(lv_rsp.iv_pid, pv_pid); // sw fault
#ifdef SQ_PHANDLE_VERIFIER
        SB_util_assert(lv_rsp.iv_verif == pv_verif); // sw fault
#endif
        pv_nid = pv_nid; // touch (in case assert disabled)
        pv_pid = pv_pid; // touch (in case assert disabled)
#ifdef SQ_PHANDLE_VERIFIER
        pv_verif = pv_verif; // touch (in case assert disabled)
#endif
    } else
        pp_sock->destroy();
    return lv_fserr;
}

//
// Purpose: negotiate (server-side)
//
int msg_mon_accept_sock_negotiate_id_srv(SB_Trans::Sock_User *pp_sock,
                                         int                 *pp_nid,
                                         int                 *pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         SB_Verif_Type       *pp_verif,
#endif
                                         char                *pp_pname,
                                         char                *pp_prog,
                                         bool                *pp_ic) {
    const char  *WHERE = "msg_mon_accept_sock_negotiate_id_srv";
    Ms_Neg_Type  lv_req;
    Ms_Neg_Type  lv_rsp;

    pp_pname[0] = '\0';
    pp_sock->read(&lv_req, sizeof(lv_req), NULL);
    if (gv_ms_trace_mon)
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(WHERE, "negotiated opener p-id=%d/%d/" PFVY ", pname=%s, prog=%s, ic=%d\n",
#else
        trace_where_printf(WHERE, "negotiated opener p-id=%d/%d, pname=%s, prog=%s, ic=%d\n",
#endif
                           lv_req.iv_nid,
                           lv_req.iv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                           lv_req.iv_verif,
#endif
                           lv_req.ia_pname,
                           lv_req.ia_prog,
                           lv_req.iv_ic);
    *pp_nid = lv_req.iv_nid;
    *pp_pid = lv_req.iv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    *pp_verif = lv_req.iv_verif;
#endif
    *pp_ic = lv_req.iv_ic;
    pp_pname[MS_MON_MAX_PROCESS_NAME-1] = '\0';
    strncpy(pp_pname, lv_req.ia_pname, MS_MON_MAX_PROCESS_NAME-1);
    pp_pname[SB_MAX_PROG-1] = '\0';
    strncpy(pp_prog, lv_req.ia_prog, SB_MAX_PROG-1);
    lv_rsp.iv_nid = gv_ms_su_nid;
    lv_rsp.iv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lv_rsp.iv_verif = gv_ms_su_verif;
#endif
    strncpy(lv_rsp.ia_pname, pp_pname, MS_MON_MAX_PROCESS_NAME);
    strncpy(lv_rsp.ia_prog, ga_ms_su_prog, SB_MAX_PROG);
    lv_rsp.iv_fserr = XZFIL_ERR_OK;
    pp_sock->write(&lv_rsp, sizeof(lv_rsp), NULL);
    return 0;
}

//
// Purpose: cleanup miscellaneous memory
//
static void msg_mon_cleanup_mem() {
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPVmap_Enum *lp_enum;
#else
    SB_LLmap_Enum *lp_enum;
#endif
    Ms_NPS_Node   *lp_node;
    void          *lp_nodev;
    bool           lv_done;

    // cleanup gv_ms_conn_state_map
    lv_done = false;
    do {
        lp_enum = gv_ms_conn_state_map.keys();
        if (lp_enum->more()) {
#ifdef SQ_PHANDLE_VERIFIER
            lp_nodev = gv_ms_conn_state_map.remove(lp_enum->next()->iv_id.npv);
#else
            lp_nodev = gv_ms_conn_state_map.remove(lp_enum->next()->iv_id.ll);
#endif
            lp_node = static_cast<Ms_NPS_Node *>(lp_nodev);
            delete lp_node;
        } else
            lv_done = true;
        delete lp_enum;
    } while (!lv_done);
}

//
// Purpose: handle closing process
//
SB_Export int msg_mon_close_process(SB_Phandle_Type *pp_phandle) {
    SB_API_CTR (lv_zctr, MSG_MON_CLOSE_PROCESS);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_CLOSE_PROCESS,
                       ms_od_map_phandle_to_oid(pp_phandle));
    return msg_mon_close_process_com(pp_phandle, true);
}

void msg_mon_close_process_idle_to_cbt(long  pv_user_param,
                                       void *pp_stream) {
    const char *WHERE = "msg_mon_close_process_idle_to_cbt";
    Ms_Od_Type *lp_od;

    lp_od = reinterpret_cast<Ms_Od_Type *>(pv_user_param);
    pp_stream = pp_stream; // touch
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "idle-timeout, od=%p, pname=%s, ref=%d\n",
                           pfp(lp_od),
                           lp_od->ia_process_name,
                           lp_od->iv_ref_count);
    msg_mon_close_process_od(lp_od, true, false);
}

int msg_mon_close_process_com(SB_Phandle_Type *pp_phandle,
                              bool             pv_free_oid) {
    const char              *WHERE = "msg_mon_close_process";
    Ms_Od_Type              *lp_od;
    SB_TML_Type             *lp_idle_link;
    SB_Trans::Trans_Stream  *lp_stream;
    bool                     lv_free_od;
    int                      lv_fserr;
    SB_Thread::Scoped_Mutex  lv_mutex_scoped(gv_ms_close_process_mutex);

    if (gv_ms_trace_mon) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, pp_phandle);
        trace_where_printf(WHERE, "ENTER phandle=%s, free-oid=%d\n",
                           la_phandle, pv_free_oid);
    }
    if (!gv_ms_calls_ok) // msg_mon_close_process
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    lp_od = ms_od_map_phandle_to_od(pp_phandle);
    if (pv_free_oid) {
        if ((lp_od != NULL) && (!gv_ms_conn_reuse)) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "no conn-reuse\n");
        } else if (lp_od != NULL) {
            int lv_ref = ms_od_ref_dec(lp_od);
            if (lp_od->iv_fs_open)
                lp_od->iv_fs_closed = true;
            lp_stream = lp_od->ip_stream;
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "no monitor close (inline), stream=%p, references=%d\n",
                                   pfp(lp_stream), lv_ref);
            lv_free_od = false;
            if ((lp_stream != NULL) && (lv_ref == 0)) {
                lp_od->iv_need_open = true;
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "send inline close\n");
                // send close to remote
                SB_Trans::Msg_Mgr::reset_md(gp_ms_rsvd_md,
                                            lp_stream,
                                            NULL,
                                            "close (msg_mon_close_process)");
                gp_ms_rsvd_md->iv_inline = true;
                lv_fserr = lp_stream->exec_close(gp_ms_rsvd_md,
                                                 gp_ms_rsvd_md->iv_link.iv_id.i); // reqid
                if (lv_fserr == XZFIL_ERR_OK) {
                    lp_stream->wait_req_done(gp_ms_rsvd_md);
                    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_REQ_DONE,
                                       gp_ms_rsvd_md->iv_link.iv_id.i);
                    lv_fserr = gp_ms_rsvd_md->out.iv_fserr;
                }
                lp_stream->md_ref_dec(); // rsvd doesn't get put
                gp_ms_rsvd_md->ip_stream = NULL;
                if (lv_fserr == XZFIL_ERR_OK) {
                    if (gv_ms_trace)
                        trace_where_printf(WHERE, "close reply-done=1\n");
                    if (gv_ms_conn_idle_timeout > 0) {
                        lp_idle_link = lp_stream->get_idle_timer_link();
                        gv_ms_idle_timer_map.lock();
                        gv_ms_idle_timer_map.remove_lock(lp_idle_link, false);
                        gv_ms_idle_timer_map.put_lock(lp_idle_link,
                                                      lp_stream, // item
                                                      reinterpret_cast<long>(lp_od), // user-param
                                                      msg_mon_close_process_idle_to_cbt,
                                                      gv_ms_conn_idle_timeout,  // tics
                                                      false); // lock
                        gv_ms_idle_timer_map.unlock();
                    }
                } else {
                    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_EXIT, lv_fserr);
                    if (lp_od->iv_fs_open)
                        lv_free_od = true;;
                }
                MS_BUF_MGR_FREE(gp_ms_rsvd_md->out.ip_recv_ctrl);
                gp_ms_rsvd_md->out.ip_recv_ctrl = NULL;
            } else if ((lp_stream == NULL) && (lv_ref == 0)) {
                if (lp_od->iv_fs_open)
                    lv_free_od = true;
            }
            if (gv_ms_trans_sock && !lp_od->iv_fs_open) {
                if ((lp_stream != NULL) && lp_stream->is_self()) {
                } else
                    lv_free_od = true;
            }
            if (!lv_free_od) {
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "EXIT OK\n");
                return XZFIL_ERR_OK;
            }
        }
    }

    if (lp_od == NULL)
        return ms_err_rtn_msg(WHERE, "invalid phandle", XZFIL_ERR_BOUNDSERR);
    return msg_mon_close_process_od(lp_od, pv_free_oid, false);
}

int msg_mon_close_process_name(const char *pp_name, bool *pp_local) {
    const char   *WHERE = "msg_mon_close_process";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Close;
    lp_msg->u.request.u.close.nid = gv_ms_su_nid;
    lp_msg->u.request.u.close.pid = gv_ms_su_pid;
    lp_msg->u.request.u.close.aborted = 0;
    lp_msg->u.request.u.close.mon = 0;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.close.verifier = gv_ms_su_verif;
#endif
    ms_util_string_copy(lp_msg->u.request.u.close.process_name,
                        sizeof(lp_msg->u.request.u.close.process_name),
                        pp_name);
    if (pp_local != NULL)
        *pp_local = true;
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send close req to mon, p-id=%d/%d/" PFVY ", pname=%s\n",
                           gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif, pp_name);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send close req to mon, p-id=%d/%d, pname=%s\n",
                           gv_ms_su_nid, gv_ms_su_pid, pp_name);
#endif
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "close",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "close req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK close req, p-id=%d/%d\n",
                                   lp_msg->u.reply.u.generic.nid,
                                   lp_msg->u.reply.u.generic.pid);
            if (pp_local != NULL)
                *pp_local = false;
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE close req, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return lv_mpierr;
}

int msg_mon_close_process_od(Ms_Od_Type *pp_od,
                             bool        pv_free_oid,
                             bool        pv_send_mon) {
    const char *WHERE = "msg_mon_close_process_od";
    bool        lv_local;
    int         lv_mpierr;

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER od=%p, pname=%s\n",
                           pfp(pp_od), pp_od->ia_process_name);
    if (pp_od->ip_stream == NULL) {
        ms_od_free(pp_od, pv_free_oid, false, false);
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXIT OK no stream, no actual close done\n");
        return XZFIL_ERR_OK;
    }

    if (!pp_od->iv_self && pv_send_mon)
        lv_mpierr = msg_mon_close_process_name(pp_od->ia_process_name, &lv_local);
    else {
        lv_mpierr = MPI_SUCCESS;
        lv_local = true;
    }

    // regardless of outcome
    ms_od_free(pp_od, pv_free_oid, lv_local, false);
    // If error clear it
    if (lv_mpierr != MPI_SUCCESS)
        lv_mpierr = MPI_SUCCESS;
    return ms_err_mpi_rtn_msg_noassert(WHERE, "EXIT", lv_mpierr);
}

int msg_mon_close_process_oid(int  pv_oid,
                              bool pv_free_oid,
                              bool pv_send_mon) {
    const char *WHERE = "msg_mon_close_process_oid";
    Ms_Od_Type *lp_od;
    int         lv_fserr;

    lp_od = gv_ms_od_mgr.get_entry(pv_oid);
    if ((lp_od != NULL) && (lp_od->iv_inuse))
        lv_fserr = msg_mon_close_process_od(lp_od, pv_free_oid, pv_send_mon);
    else {
        if (gv_ms_trace)
            trace_where_printf(WHERE, "oid=%d does not have valid od\n", pv_oid);
        lv_fserr = XZFIL_ERR_NOTFOUND;
    }
    return lv_fserr;
}

//
// Purpose: Change connection state (return true if open)
//
static bool msg_mon_conn_state_change(int            pv_nid,
                                      int            pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                      SB_Verif_Type  pv_verif,
#endif
                                      MSGTYPE        pv_msg_type) {
    const char     *WHERE = "msg_mon_conn_state_change";
    Ms_NPS_Node    *lp_node;
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPV_Type     lv_npv;
#else
    Ms_NidPid_Type  lv_nidpid;
#endif
    bool            lv_ret;

    if (gv_ms_trace) {
        const char *lp_msg_type = msg_util_get_msg_type(pv_msg_type);
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(WHERE, "ENTER p-id=%d/%d/" PFVY ", msgtype=%s(%d)\n",
#else
        trace_where_printf(WHERE, "ENTER p-id=%d/%d, msgtype=%s(%d)\n",
#endif
                           pv_nid, pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                           pv_verif,
#endif
                           lp_msg_type, pv_msg_type);
    }
#ifdef SQ_PHANDLE_VERIFIER
    lv_npv.iv_nid = pv_nid;
    lv_npv.iv_pid = pv_pid;
    lv_npv.iv_verif = pv_verif;
#else
    lv_nidpid.u.i.iv_nid = pv_nid;
    lv_nidpid.u.i.iv_pid = pv_pid;
#endif
    if (pv_msg_type == MsgType_Close) {
#ifdef SQ_PHANDLE_VERIFIER
        lp_node =
          static_cast<Ms_NPS_Node *>(gv_ms_conn_state_map.remove(lv_npv));
#else
        lp_node =
          static_cast<Ms_NPS_Node *>(gv_ms_conn_state_map.remove(lv_nidpid.u.iv_nidpid));
#endif
        if (lp_node != NULL) {
            delete lp_node;
            lv_ret = true;
        } else
            lv_ret = false;
    } else {
#ifdef SQ_PHANDLE_VERIFIER
        lp_node = new_NPS_Node(lv_npv);
#else
        lp_node = new_NPS_Node(lv_nidpid.u.iv_nidpid);
#endif
        gv_ms_conn_state_map.put(&lp_node->iv_link);
        lp_node->iv_state = pv_msg_type;
        lv_ret = true;
    }
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_ret);
    return lv_ret;
}

//
// Purpose: get connection state (return true if open)
//
#ifdef SQ_PHANDLE_VERIFIER
static bool msg_mon_conn_state_get(int pv_nid, int pv_pid, SB_Verif_Type pv_verif) {
#else
static bool msg_mon_conn_state_get(int pv_nid, int pv_pid) {
#endif
    const char     *WHERE = "msg_mon_conn_state_get";
    Ms_NPS_Node    *lp_node;
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPV_Type     lv_npv;
#else
    Ms_NidPid_Type  lv_nidpid;
#endif
    bool            lv_ret;

    if (gv_ms_trace)
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(WHERE, "ENTER p-id=%d/%d/" PFVY "\n", pv_nid, pv_pid, pv_verif);
#else
        trace_where_printf(WHERE, "ENTER p-id=%d/%d\n", pv_nid, pv_pid);
#endif
#ifdef SQ_PHANDLE_VERIFIER
    lv_npv.iv_nid = pv_nid;
    lv_npv.iv_pid = pv_pid;
    lv_npv.iv_verif = pv_verif;
    lp_node =
      static_cast<Ms_NPS_Node *>(gv_ms_conn_state_map.get(lv_npv));
#else
    lv_nidpid.u.i.iv_nid = pv_nid;
    lv_nidpid.u.i.iv_pid = pv_pid;
    lp_node =
      static_cast<Ms_NPS_Node *>(gv_ms_conn_state_map.get(lv_nidpid.u.iv_nidpid));
#endif
    lv_ret = (lp_node != NULL);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_ret);
    return lv_ret;
}

#ifdef SQ_PHANDLE_VERIFIER
SB_Export int msg_mon_create_name_seq(char          *pp_name,
                                      SB_Verif_Type  pv_verifier,
                                      char          *pp_name_seq,
                                      int            pv_name_seq_len) {
    const char *WHERE = "msg_mon_create_name_seq";
    char        la_seq[20];
    int         lv_len;
    int         lv_seq_len;

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER name=%s, verifier=" PFVY ", name-seq=%p, name-seq-len=%d\n",
                           pp_name, pv_verifier, pfp(pp_name_seq), pv_name_seq_len);

    lv_len = sprintf(la_seq, PFVY, pv_verifier);
    lv_seq_len = static_cast<int>(strlen(pp_name) + // "$xyz"
                                  1 +               // ":"
                                  lv_len +          // <seq>
                                  1);               // <eoln>

    if (pv_name_seq_len < lv_seq_len)
        return XZFIL_ERR_BOUNDSERR;

    sprintf(pp_name_seq, "%s:%s", pp_name, la_seq);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK name-seq=%s\n", pp_name_seq);
    return XZFIL_ERR_OK;
}
#endif

//
// Purpose: delete tag
//
void msg_mon_delete_tag(int pv_tag) {
    Map_Tag_Entry_Type *lp_entry =
      static_cast<Map_Tag_Entry_Type *>(gv_ms_tag_map.remove(pv_tag));
    if (lp_entry != NULL)
        delete lp_entry;
}

//
// Purpose: deregister for death notifications
//
SB_Export int msg_mon_deregister_death_notification(int                 pv_target_nid,
                                                    int                 pv_target_pid,
                                                    MS_Mon_Transid_Type pv_target_transid) {
    const char *WHERE = "msg_mon_deregister_death_notification";
    SB_API_CTR (lv_zctr, MSG_MON_DEREGISTER_DEATH_NOTIFICATION);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_DEREG_DEATH_NOTIFICATION, 0);
    if (gv_ms_trace_mon) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_target_transid);
        trace_where_printf(WHERE, "ENTER target-p-id=%d/%d, target-transid=%s\n",
                           pv_target_nid, pv_target_pid, la_transid);
    }
    if (!gv_ms_mon_calls_ok) // msg_mon_deregister_death_notification
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    int lv_fserr = msg_mon_send_notify(WHERE, gv_ms_su_nid, gv_ms_su_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       true,
                                       pv_target_nid, pv_target_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       pv_target_transid, true);
    return lv_fserr;
}

//
// Purpose: deregister for death notifications
//
SB_Export int msg_mon_deregister_death_notification2(int                 pv_notify_nid,
                                                     int                 pv_notify_pid,
                                                     int                 pv_target_nid,
                                                     int                 pv_target_pid,
                                                     MS_Mon_Transid_Type pv_target_transid) {
    const char *WHERE = "msg_mon_deregister_death_notification2";
    SB_API_CTR (lv_zctr, MSG_MON_DEREGISTER_DEATH_NOTIFICATION2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_DEREG_DEATH_NOTIFICATION2, 0);
    if (gv_ms_trace_mon) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_target_transid);
        trace_where_printf(WHERE, "ENTER notify-p-id=%d/%d, target-p-id=%d/%d, target-transid=%s\n",
                           pv_notify_nid, pv_notify_pid,
                           pv_target_nid, pv_target_pid,
                           la_transid);
    }
    if (!gv_ms_mon_calls_ok) // msg_mon_deregister_death_notification2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    int lv_fserr = msg_mon_send_notify(WHERE, pv_notify_nid, pv_notify_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       true,
                                       pv_target_nid, pv_target_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       pv_target_transid, true);
    return lv_fserr;
}

#ifdef SQ_PHANDLE_VERIFIER
//
// Purpose: deregister for death notifications
//
SB_Export int msg_mon_deregister_death_notification3(int                 pv_notify_nid,
                                                     int                 pv_notify_pid,
                                                     SB_Verif_Type       pv_notify_verifier,
                                                     int                 pv_target_nid,
                                                     int                 pv_target_pid,
                                                     SB_Verif_Type       pv_target_verifier,
                                                     MS_Mon_Transid_Type pv_target_transid) {
    const char *WHERE = "msg_mon_deregister_death_notification3";
    SB_API_CTR (lv_zctr, MSG_MON_DEREGISTER_DEATH_NOTIFICATION3);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_DEREG_DEATH_NOTIFICATION3, 0);
    if (gv_ms_trace_mon) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_target_transid);
        trace_where_printf(WHERE, "ENTER notify-p-id=%d/%d/" PFVY ", target-p-id=%d/%d/" PFVY ", target-transid=%s\n",
                           pv_notify_nid, pv_notify_pid, pv_notify_verifier,
                           pv_target_nid, pv_target_pid, pv_target_verifier,
                           la_transid);
    }
    if (!gv_ms_mon_calls_ok) // msg_mon_deregister_death_notification2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    int lv_fserr = msg_mon_send_notify(WHERE, pv_notify_nid, pv_notify_pid, pv_notify_verifier,
                                       true,
                                       pv_target_nid, pv_target_pid, pv_target_verifier,
                                       pv_target_transid, true);
    return lv_fserr;
}

//
// Purpose: deregister for death notifications
//
SB_Export int msg_mon_deregister_death_notification_name(const char          *pp_target_name,
                                                         MS_Mon_Transid_Type  pv_target_transid) {
    const char *WHERE = "msg_mon_deregister_death_notification_name";
    SB_API_CTR (lv_zctr, MSG_MON_DEREGISTER_DEATH_NOTIFICATION_NAME);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_DEREG_DEATH_NOTIFICATION_NAME, 0);
    if (gv_ms_trace_mon) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_target_transid);
        trace_where_printf(WHERE, "ENTER target-name=%s, target-transid=%s\n",
                           pp_target_name, la_transid);
    }
    if (!gv_ms_mon_calls_ok) // msg_mon_deregister_death_notification
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_target_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid target name (null)",
                              XZFIL_ERR_BOUNDSERR);
    int lv_fserr = msg_mon_send_notify_verif(WHERE, ga_ms_su_pname_seq, true,
                                             pp_target_name,
                                             pv_target_transid, true);
    return lv_fserr;
}

//
// Purpose: deregister for death notifications
//
SB_Export int msg_mon_deregister_death_notification_name2(const char          *pp_notify_name,
                                                          const char          *pp_target_name,
                                                          MS_Mon_Transid_Type  pv_target_transid) {
    const char *WHERE = "msg_mon_deregister_death_notification_name2";
    SB_API_CTR (lv_zctr, MSG_MON_DEREGISTER_DEATH_NOTIFICATION_NAME2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_DEREG_DEATH_NOTIFICATION_NAME2, 0);
    if (gv_ms_trace_mon) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_target_transid);
        trace_where_printf(WHERE, "ENTER notify-name=%s, target-name=%s, target-transid=%s\n",
                           pp_notify_name, pp_target_name, la_transid);
    }
    if (!gv_ms_mon_calls_ok) // msg_mon_deregister_death_notification2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_notify_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid notify name (null)",
                              XZFIL_ERR_BOUNDSERR);
    if (pp_target_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid target name (null)",
                              XZFIL_ERR_BOUNDSERR);
    int lv_fserr = msg_mon_send_notify_verif(WHERE, pp_notify_name,
                                             true,
                                             pp_target_name,
                                             pv_target_transid, true);
    return lv_fserr;
}
#endif

#if 0
//
// Purpose: display tags
//
static void msg_mon_display_tags() {
    SB_Imap_Enum *lp_enum = gv_ms_tag_map.keys();
    printf("display tags\n");
    while (lp_enum->more()) {
        int lv_tag = lp_enum->next()->iv_id.i;
        printf("tag=%d\n", lv_tag);
    }
    delete lp_enum;
}
#endif


static int msg_mon_dump_process(const char    *pp_where,
                                const char    *pp_path,
                                const char    *pp_name,
                                int            pv_nid,
                                int            pv_pid,
                                SB_Verif_Type  pv_verif,
                                char          *pp_core_file) {
    char         *lp_cwd;
    Mon_Msg_Type *lp_msg;
    const char   *lp_path;
    size_t        lv_len;
    int           lv_mpierr;
    Mon_Msg_Auto  lv_msg;

    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Dump;
    lp_msg->u.request.u.dump.nid = gv_ms_su_nid;
    lp_msg->u.request.u.dump.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.dump.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.dump.process_name,
                         sizeof(lp_msg->u.request.u.dump.process_name));
#endif
    if (pp_path == NULL) {
        lp_path = getenv(gp_ms_env_sq_snapshot_dir);
        if (lp_path == NULL)
            lp_path = getenv("TRAF_LOG");
    } else
        lp_path = pp_path;
    // make absolute path and check len - don't need ms_util_string_copy
    if (lp_path[0] == '/') {
        lv_len = strlen(lp_path);
        if (lv_len > sizeof(lp_msg->u.request.u.dump.path))
            return ms_err_rtn_msg(pp_where, "EXIT", XZFIL_ERR_BOUNDSERR);
        strcpy(lp_msg->u.request.u.dump.path, lp_path);
    } else {
        lp_cwd = getenv("TRAF_LOG");
        lv_len = strlen(lp_cwd) + 1 + strlen(lp_path);
        if (lv_len > sizeof(lp_msg->u.request.u.dump.path))
            return ms_err_rtn_msg(pp_where, "EXIT", XZFIL_ERR_BOUNDSERR);
        sprintf(lp_msg->u.request.u.dump.path, "%s/%s", lp_cwd, lp_path);
    }
    lp_msg->u.request.u.dump.target_nid = pv_nid;
    lp_msg->u.request.u.dump.target_pid = pv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.dump.target_verifier = pv_verif;
#else
    pv_verif = pv_verif; // touch
#endif
    if (pp_name == NULL)
        ms_util_string_clear(lp_msg->u.request.u.dump.target_process_name,
                             sizeof(lp_msg->u.request.u.dump.target_process_name));
    else
        ms_util_string_copy(lp_msg->u.request.u.dump.target_process_name,
                            sizeof(lp_msg->u.request.u.dump.target_process_name),
                            pp_name);
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "send dump req to mon, p-id=%d/%d/" PFVY ", path=%s, t-p-id=%d/%d/" PFVY ", t-name=%s:" PFVY "\n",
                           gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif,
                           lp_msg->u.request.u.dump.path,
                           pv_nid, pv_pid,
                           lp_msg->u.request.u.dump.target_verifier,
                           lp_msg->u.request.u.dump.target_process_name,
                           lp_msg->u.request.u.dump.target_verifier);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "send dump req to mon, p-id=%d/%d, path=%s, t-p-id=%d/%d, t-name=%s\n",
                           gv_ms_su_nid, gv_ms_su_pid,
                           lp_msg->u.request.u.dump.path,
                           pv_nid, pv_pid, pp_name);
#endif
    lv_mpierr = msg_mon_sendrecv_mon(pp_where,
                                     "dump",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(pp_where,
                       "dump req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Dump)) {
        lv_mpierr = lp_msg->u.reply.u.dump.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(pp_where, "EXIT OK dump req, core-file=%s\n",
                                   lp_msg->u.reply.u.dump.core_file);
            if (pp_core_file != NULL)
                strcpy(pp_core_file, lp_msg->u.reply.u.dump.core_file);
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(pp_where, "EXIT FAILURE dump, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(pp_where, "EXIT", lv_mpierr);
}

//
// Purpose: dump process by id
//
SB_Export int msg_mon_dump_process_id(const char *pp_path,
                                      int         pv_nid,
                                      int         pv_pid,
                                      char       *pp_core_file) {
    const char *WHERE = "msg_mon_dump_process_id";
    SB_API_CTR (lv_zctr, MSG_MON_DUMP_PROCESS_ID);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_DUMP_PROCESS_ID, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER path=%s, p-id=%d/%d, core-file=%p\n",
                           pp_path, pv_nid, pv_pid, pp_core_file);
    if (!gv_ms_mon_calls_ok) // msg_mon_dump_process_id
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    return msg_mon_dump_process(WHERE,
                                pp_path,
                                NULL,
                                pv_nid,
                                pv_pid,
                                -1, // TODO: use verifier
                                pp_core_file);
}

//
// Purpose: dump process by name
//
SB_Export int msg_mon_dump_process_name(const char *pp_path,
                                        const char *pp_name,
                                        char       *pp_core_file) {
    const char *WHERE = "msg_mon_dump_process_name";
    SB_API_CTR (lv_zctr, MSG_MON_DUMP_PROCESS_NAME);
#ifdef SQ_PHANDLE_VERIFIER
    char           la_name[MAX_PROCESS_NAME];
    SB_Verif_Type  lv_verif;
#endif

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_DUMP_PROCESS_NAME, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER path=%s, name=%s, core-file=%p\n",
                           pp_path, pp_name, pp_core_file);
    if (!gv_ms_mon_calls_ok) // msg_mon_dump_process_name
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid name (null)",
                              XZFIL_ERR_BOUNDSERR);
#ifdef SQ_PHANDLE_VERIFIER
    ms_util_name_seq(pp_name,
                     la_name,
                     sizeof(la_name),
                     &lv_verif);
    return msg_mon_dump_process(WHERE,
                                pp_path,
                                la_name,
                                -1,
                                -1,
                                lv_verif,
                                pp_core_file);
#else
    return msg_mon_dump_process(WHERE,
                                pp_path,
                                pp_name,
                                -1,
                                -1,
                                -1,
                                pp_core_file);
#endif
}

//
// Purpose: enable monitor messages
//
SB_Export void msg_mon_enable_mon_messages(int pv_enable_messages) {
    const char *WHERE = "msg_mon_enable_mon_messages";
    SB_API_CTR (lv_zctr, MSG_MON_ENABLE_MON_MESSAGES);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_ENABLE_MON_MESSAGES, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER enable=%d\n", pv_enable_messages);
    gv_ms_enable_messages = pv_enable_messages;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");
}

//
// Purpose: send event
//
SB_Export int msg_mon_event_send(int   pv_nid,
                                 int   pv_pid,
                                 int   pv_process_type,
                                 int   pv_event_id,
                                 int   pv_event_len,
                                 char *pp_event_data) {
    const char   *WHERE = "msg_mon_event_send";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_EVENT_SEND);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_EVENT_SEND, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER p-id=%d/%d, process-type=%d, event_id=%d, event_len=%d, event_data=%p\n",
                           pv_nid, pv_pid, pv_process_type,
                           pv_event_id, pv_event_len, pp_event_data);
    if (!gv_ms_mon_calls_ok) // msg_mon_event_send
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if ((pv_process_type < MS_ProcessType_Undefined) ||
        (pv_process_type > MS_ProcessType_SMS))
        return ms_err_rtn_msg(WHERE, "invalid process-type",
                              XZFIL_ERR_BOUNDSERR);
    if (pv_event_len > 0) {
        if (pv_event_len > MAX_SYNC_DATA)
            return ms_err_rtn_msg(WHERE, "invalid event-data (too big)",
                                  XZFIL_ERR_BOUNDSERR);
        if (pp_event_data == NULL)
            return ms_err_rtn_msg(WHERE, "invalid event-data (null)",
                                  XZFIL_ERR_BOUNDSERR);
    }

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Event;
    lp_msg->u.request.u.event.nid = gv_ms_su_nid;
    lp_msg->u.request.u.event.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.event.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.event.process_name,
                         sizeof(lp_msg->u.request.u.event.process_name));
#endif
    lp_msg->u.request.u.event.target_nid = pv_nid;
    lp_msg->u.request.u.event.target_pid = pv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.event.target_verifier = -1; // OLD i/f
    ms_util_string_clear(lp_msg->u.request.u.event.target_process_name,
                         sizeof(lp_msg->u.request.u.event.target_process_name));
#endif
    lp_msg->u.request.u.event.type = static_cast<PROCESSTYPE>(pv_process_type);
    lp_msg->u.request.u.event.event_id = pv_event_id;
    lp_msg->u.request.u.event.length = pv_event_len;
    if (pv_event_len > 0)
        memcpy(lp_msg->u.request.u.event.data, pp_event_data, pv_event_len);
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send event req to mon, p-id=%d/%d/" PFVY ", t-id=%d/%d\n",
                           gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif, pv_nid, pv_pid);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send event req to mon, p-id=%d/%d, t-id=%d/%d\n",
                           gv_ms_su_nid, gv_ms_su_pid, pv_nid, pv_pid);
#endif
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "event",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "event req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK event req\n");
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE event, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

#ifdef SQ_PHANDLE_VERIFIER
//
// Purpose: send event
//
SB_Export int msg_mon_event_send_name(const char *pp_name,
                                      int         pv_process_type,
                                      int         pv_event_id,
                                      int         pv_event_len,
                                      char       *pp_event_data) {
    const char   *WHERE = "msg_mon_event_send_name";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_EVENT_SEND_NAME);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_EVENT_SEND_NAME, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER name=%s, process-type=%d, event_id=%d, event_len=%d, event_data=%p\n",
                           pp_name, pv_process_type,
                           pv_event_id, pv_event_len, pp_event_data);
    if (!gv_ms_mon_calls_ok) // msg_mon_event_send
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid name (null)",
                              XZFIL_ERR_BOUNDSERR);
    if ((pv_process_type < MS_ProcessType_Undefined) ||
        (pv_process_type > MS_ProcessType_SMS))
        return ms_err_rtn_msg(WHERE, "invalid process-type",
                              XZFIL_ERR_BOUNDSERR);
    if (pv_event_len > 0) {
        if (pv_event_len > MAX_SYNC_DATA)
            return ms_err_rtn_msg(WHERE, "invalid event-data (too big)",
                                  XZFIL_ERR_BOUNDSERR);
        if (pp_event_data == NULL)
            return ms_err_rtn_msg(WHERE, "invalid event-data (null)",
                                  XZFIL_ERR_BOUNDSERR);
    }

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Event;
    lp_msg->u.request.u.event.nid = gv_ms_su_nid;
    lp_msg->u.request.u.event.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.event.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.event.process_name,
                         sizeof(lp_msg->u.request.u.event.process_name));
#endif
    lp_msg->u.request.u.event.target_nid = -2;
    lp_msg->u.request.u.event.target_pid = -2;
    ms_util_name_seq(pp_name,
                     lp_msg->u.request.u.event.target_process_name,
                     sizeof(lp_msg->u.request.u.event.target_process_name),
                     &lp_msg->u.request.u.event.target_verifier);
    lp_msg->u.request.u.event.type = static_cast<PROCESSTYPE>(pv_process_type);
    lp_msg->u.request.u.event.event_id = pv_event_id;
    lp_msg->u.request.u.event.length = pv_event_len;
    if (pv_event_len > 0)
        memcpy(lp_msg->u.request.u.event.data, pp_event_data, pv_event_len);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send event req to mon, p-id=%d/%d/" PFVY ", t-name=%s:" PFVY "\n",
                           gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif,
                           lp_msg->u.request.u.event.target_process_name,
                           lp_msg->u.request.u.event.target_verifier);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "event",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "event req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK event req\n");
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE event, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}
#endif

//
// Purpose: wait for event
//
SB_Export int msg_mon_event_wait(int   pv_event_id,
                                 int  *pp_event_len,
                                 char *pp_event_data) {
    const char   *WHERE = "msg_mon_event_wait";
    Mon_Msg_Type *lp_msg = NULL;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_EVENT_WAIT);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_EVENT_WAIT, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER event_id=%d, event_len=%p, event_data=%p\n",
                           pv_event_id, pfp(pp_event_len), pp_event_data);
    if (!gv_ms_mon_calls_ok) // msg_mon_event_wait
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg(NULL); // no msg - set later
    if (gp_local_mon_io->wait_for_event(&lp_msg)) {
        lv_mpierr = ms_err_errno_to_mpierr(WHERE);
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_WAIT_FOR_EVENT,
                           errno,
                           lv_mpierr);
        return ms_err_mpi_rtn_msg(WHERE, "wait_for_event() failed",
                                  lv_mpierr);
    }
    lv_msg.set_msg(lp_msg);
    bool lv_ok = (lp_msg->type == MsgType_Event) &&
                 (lp_msg->u.request.type == ReqType_Notice) &&
                 (lp_msg->u.request.u.event_notice.event_id == pv_event_id);
    if (!lv_ok) {
        SB_Buf_Line la_line;
        if (lp_msg->type != MsgType_Event)
            sprintf(la_line, "msg.type(%d) != MsgType_Event\n", lp_msg->type);
        else if (lp_msg->u.request.type != ReqType_Notice)
            sprintf(la_line, "msg.request.type(%d) != ReqType_Notice\n",
                    lp_msg->u.request.type);
        else if (lp_msg->u.request.u.event_notice.event_id != pv_event_id)
            sprintf(la_line, "msg.request.event_id(%d) != %d\n",
                    lp_msg->u.request.u.event_notice.event_id, pv_event_id);
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXIT FAILURE %s\n", la_line);
        return ms_err_rtn_msg(WHERE, la_line, XZFIL_ERR_INVALOP);
    }
    if (gv_ms_trace_mon)
        trace_print_data(lp_msg->u.request.u.event_notice.data,
                         lp_msg->u.request.u.event_notice.length,
                         lp_msg->u.request.u.event_notice.length);
    if (pp_event_len != NULL)
        *pp_event_len = lp_msg->u.request.u.event_notice.length;
    if (pp_event_data != NULL)
        memcpy(pp_event_data,
               lp_msg->u.request.u.event_notice.data,
               lp_msg->u.request.u.event_notice.length);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");

    return XZFIL_ERR_OK;
}

//
// Purpose: wait for event
//
SB_Export int msg_mon_event_wait2(int  *pp_event_id,
                                  int  *pp_event_len,
                                  char *pp_event_data) {
    const char   *WHERE = "msg_mon_event_wait2";
    Mon_Msg_Type *lp_msg = NULL;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_EVENT_WAIT2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_EVENT_WAIT2, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER event_len=%p, event_data=%p, event_id=%p\n",
                           pfp(pp_event_id), pfp(pp_event_len), pp_event_data);
    if (!gv_ms_mon_calls_ok) // msg_mon_event_wait2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg(NULL); // no msg - set later
    if (gp_local_mon_io->wait_for_event(&lp_msg)) {
        lv_mpierr = ms_err_errno_to_mpierr(WHERE);
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_WAIT_FOR_EVENT,
                           errno,
                           lv_mpierr);
        return ms_err_mpi_rtn_msg(WHERE, "wait_for_event() failed",
                                  lv_mpierr);
    }
    lv_msg.set_msg(lp_msg);
    bool lv_ok = (lp_msg->type == MsgType_Event) &&
                 (lp_msg->u.request.type == ReqType_Notice);
    if (!lv_ok) {
        SB_Buf_Line la_line;
        if (lp_msg->type != MsgType_Event)
            sprintf(la_line, "msg.type(%d) != MsgType_Event\n", lp_msg->type);
        else if (lp_msg->u.request.type != ReqType_Notice)
            sprintf(la_line, "msg.request.type(%d) != ReqType_Notice\n",
                    lp_msg->u.request.type);
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXIT FAILURE %s\n", la_line);
        return ms_err_rtn_msg(WHERE, la_line, XZFIL_ERR_INVALOP);
    }
    if (gv_ms_trace_mon) {
        trace_where_printf(WHERE, "event_id=%d\n",
                           lp_msg->u.request.u.event_notice.event_id);
        trace_print_data(lp_msg->u.request.u.event_notice.data,
                         lp_msg->u.request.u.event_notice.length,
                         lp_msg->u.request.u.event_notice.length);
    }
    if (pp_event_id != NULL)
        *pp_event_id = lp_msg->u.request.u.event_notice.event_id;
    if (pp_event_len != NULL)
        *pp_event_len = lp_msg->u.request.u.event_notice.length;
    if (pp_event_data != NULL)
        memcpy(pp_event_data,
               lp_msg->u.request.u.event_notice.data,
               lp_msg->u.request.u.event_notice.length);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");

    return XZFIL_ERR_OK;
}

static void msg_mon_fs_trace_change(const char *pp_key, const char *pp_value) {
    static void              *lp_handle;
    static Trace_Change_Type  lv_change;
    static bool               lv_inited = false;

    if (!lv_inited) {
        lv_inited = true;
        lp_handle = dlopen("libsbfs.so", RTLD_NOW);
        if (lp_handle != NULL)
            lv_change.ipchange = dlsym(lp_handle, "fs_trace_change_list");
        else
            lv_change.ipchange = NULL;
    }
    ms_trace_change_list(pp_key, pp_value);
    if (memcmp(pp_key, "FS_", 3) == 0)
        if (lv_change.ipchange != NULL)
            lv_change.ichange(pp_key, pp_value);
}

//
// Purpose: get cluster id and instance id
//
SB_Export int msg_mon_get_instance_id(int *pp_cluster_id,
                                      int *pp_instance_id) {
    const char   *WHERE = "msg_mon_get_instance_id";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_INSTANCE_ID);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_INSTANCE_ID, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER nid=%d, pid=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid);
    if (!gv_ms_mon_calls_ok) // msg_mon_get_zone_info_detail
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_InstanceId;
    lp_msg->u.request.u.instance_id.nid = gv_ms_su_nid;
    lp_msg->u.request.u.instance_id.pid = gv_ms_su_pid;
    lp_msg->u.request.u.instance_id.verifier = gv_ms_su_verif;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send instance-id req to mon, nid=%d, pid=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "instance-id",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "instance-id req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_InstanceId)) {
        struct InstanceId_reply_def *lp_instance = &lp_msg->u.reply.u.instance_id;
        // copy results for user
        *pp_cluster_id = lp_instance->cluster_id;
        *pp_instance_id = lp_instance->instance_id;
        if (gv_ms_trace_mon) {
            if (lv_mpierr == MPI_SUCCESS) {
                trace_where_printf(WHERE, "EXIT OK instance-id req, cluster_id=%d, instance_id=%d\n",
                                   lp_instance->cluster_id, lp_instance->instance_id);
            } else
                trace_where_printf(WHERE, "EXIT FAILED instance-id req, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: get monitor stats
//
SB_Export int msg_mon_get_monitor_stats(MS_Mon_Monitor_Stats_Type *pp_stats) {
    const char   *WHERE = "msg_mon_get_monitor_stats";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_MONITOR_STATS);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER stats=%p\n", pfp(pp_stats));

    if (!gv_ms_mon_calls_ok) // msg_mon_get_monitor_stats
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_MonStats;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send mon-stats req to mon, p-id=%d/%d\n",
                           gv_ms_su_nid, gv_ms_su_pid);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "mon-stats",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "mon-stats req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_MonStats)) {
        struct MonStats_reply_def *lp_stats = &lp_msg->u.reply.u.mon_info;
        if (gv_ms_trace_mon) {
            if (lv_mpierr == MPI_SUCCESS) {
                trace_where_printf(WHERE, "EXIT OK mon-stats req, acquired_max=%d, avail_min=%d, buf_misses=%d\n",
                                   lp_stats->acquiredMax,
                                   lp_stats->availMin,
                                   lp_stats->bufMisses);
            } else
                trace_where_printf(WHERE, "EXIT FAILED node-info req, ret=%d\n",
                                   lv_mpierr);
        }
        // copy results for user
        memcpy(pp_stats, lp_stats, sizeof(MS_Mon_Monitor_Stats_Type));
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: get my info
//
SB_Export int msg_mon_get_my_info(int  *pp_mon_nid,
                                  int  *pp_mon_pid,
                                  char *pp_mon_name,
                                  int   pv_mon_name_len,
                                  int  *pp_mon_ptype,
                                  int  *pp_mon_zid,
                                  int  *pp_os_pid,
                                  long *pp_os_tid) {
    const char *WHERE = "msg_mon_get_my_info";
    SB_API_CTR (lv_zctr, MSG_MON_GET_MY_INFO);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER m-id=%p/%p, m-name=%p, m-name-len=%d, m-ptype=%p, m-zid=%p, os-pid=%p, os-tid=%p\n",
                           pfp(pp_mon_nid),
                           pfp(pp_mon_pid),
                           pp_mon_name,
                           pv_mon_name_len,
                           pfp(pp_mon_ptype),
                           pfp(pp_mon_zid),
                           pfp(pp_os_pid),
                           pfp(pp_os_tid));

    //
    // do not check msg_init [in case logger calls]
    //

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "m-id=%d/%d, m-name=%s, m-ptype=%d, m-zid=%d, os-pid=%d, os-tid=%d\n",
                           gv_ms_su_nid,
                           gv_ms_su_pid,
                           ga_ms_su_pname,
                           gv_ms_su_ptype,
                           gv_ms_su_zid,
                           getpid(),
                           static_cast<int>(SB_Thread::Sthr::self_id()));
    if (pp_mon_nid != NULL)
        *pp_mon_nid = gv_ms_su_nid;
    if (pp_mon_pid != NULL)
        *pp_mon_pid = gv_ms_su_pid;
    if ((pp_mon_name != NULL) && (pv_mon_name_len > 1)) {
        strncpy(pp_mon_name, ga_ms_su_pname, pv_mon_name_len);
        pp_mon_name[pv_mon_name_len-1] = '\0';
    }
    if (pp_mon_ptype != NULL)
        *pp_mon_ptype = gv_ms_su_ptype;
    if (pp_mon_zid != NULL)
        *pp_mon_zid = gv_ms_su_zid;
    if (pp_os_pid != NULL)
        *pp_os_pid = getpid();
    if (pp_os_tid != NULL)
        *pp_os_tid = SB_Thread::Sthr::self_id();
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");
    return XZFIL_ERR_OK;
}

//
// Purpose: get my info
//
SB_Export int msg_mon_get_my_info2(int  *pp_mon_nid,
                                   int  *pp_mon_pid,
                                   char *pp_mon_name,
                                   int   pv_mon_name_len,
                                   int  *pp_mon_ptype,
                                   int  *pp_mon_zid,
                                   int  *pp_os_pid,
                                   long *pp_os_tid,
                                   int  *pp_compid) {
    const char *WHERE = "msg_mon_get_my_info2";
    SB_API_CTR (lv_zctr, MSG_MON_GET_MY_INFO2);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER m-id=%p/%p, m-name=%p, m-name-len=%d, m-ptype=%p, m-zid=%p, os-pid=%p, os-tid=%p, compid=%p\n",
                           pfp(pp_mon_nid),
                           pfp(pp_mon_pid),
                           pp_mon_name,
                           pv_mon_name_len,
                           pfp(pp_mon_ptype),
                           pfp(pp_mon_zid),
                           pfp(pp_os_pid),
                           pfp(pp_os_tid),
                           pfp(pp_compid));

    //
    // do not check msg_init [in case logger calls]
    //

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "m-id=%d/%d, m-name=%s, m-ptype=%d, m-zid=%d, os-pid=%d, os-tid=%d, compid=%d\n",
                           gv_ms_su_nid,
                           gv_ms_su_pid,
                           ga_ms_su_pname,
                           gv_ms_su_ptype,
                           gv_ms_su_zid,
                           getpid(),
                           static_cast<int>(SB_Thread::Sthr::self_id()),
                           gv_ms_su_compid);
    if (pp_mon_nid != NULL)
        *pp_mon_nid = gv_ms_su_nid;
    if (pp_mon_pid != NULL)
        *pp_mon_pid = gv_ms_su_pid;
    if ((pp_mon_name != NULL) && (pv_mon_name_len > 1)) {
        strncpy(pp_mon_name, ga_ms_su_pname, pv_mon_name_len);
        pp_mon_name[pv_mon_name_len-1] = '\0';
    }
    if (pp_mon_ptype != NULL)
        *pp_mon_ptype = gv_ms_su_ptype;
    if (pp_mon_zid != NULL)
        *pp_mon_zid = gv_ms_su_zid;
    if (pp_os_pid != NULL)
        *pp_os_pid = getpid();
    if (pp_os_tid != NULL)
        *pp_os_tid = SB_Thread::Sthr::self_id();
    if (pp_compid != NULL)
        *pp_compid = gv_ms_su_compid;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");
    return XZFIL_ERR_OK;
}

//
// Purpose: get my info
//
SB_Export int msg_mon_get_my_info3(int  *pp_mon_nid,
                                   int  *pp_mon_pid,
                                   char *pp_mon_name,
                                   int   pv_mon_name_len,
                                   int  *pp_mon_ptype,
                                   int  *pp_mon_zid,
                                   int  *pp_os_pid,
                                   long *pp_os_tid,
                                   int  *pp_compid,
                                   int  *pp_pnid) {
    const char *WHERE = "msg_mon_get_my_info3";
    SB_API_CTR (lv_zctr, MSG_MON_GET_MY_INFO3);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER m-id=%p/%p, m-name=%p, m-name-len=%d, m-ptype=%p, m-zid=%p, os-pid=%p, os-tid=%p, compid=%p, pnid=%p\n",
                           pfp(pp_mon_nid),
                           pfp(pp_mon_pid),
                           pp_mon_name,
                           pv_mon_name_len,
                           pfp(pp_mon_ptype),
                           pfp(pp_mon_zid),
                           pfp(pp_os_pid),
                           pfp(pp_os_tid),
                           pfp(pp_compid),
                           pfp(pp_pnid));

    //
    // do not check msg_init [in case logger calls]
    //

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "m-id=%d/%d, m-name=%s, m-ptype=%d, m-zid=%d, os-pid=%d, os-tid=%d, compid=%d, pnid=%d\n",
                           gv_ms_su_nid,
                           gv_ms_su_pid,
                           ga_ms_su_pname,
                           gv_ms_su_ptype,
                           gv_ms_su_zid,
                           getpid(),
                           static_cast<int>(SB_Thread::Sthr::self_id()),
                           gv_ms_su_compid,
                           gv_ms_su_pnid);
    if (pp_mon_nid != NULL)
        *pp_mon_nid = gv_ms_su_nid;
    if (pp_mon_pid != NULL)
        *pp_mon_pid = gv_ms_su_pid;
    if ((pp_mon_name != NULL) && (pv_mon_name_len > 1)) {
        strncpy(pp_mon_name, ga_ms_su_pname, pv_mon_name_len);
        pp_mon_name[pv_mon_name_len-1] = '\0';
    }
    if (pp_mon_ptype != NULL)
        *pp_mon_ptype = gv_ms_su_ptype;
    if (pp_mon_zid != NULL)
        *pp_mon_zid = gv_ms_su_zid;
    if (pp_os_pid != NULL)
        *pp_os_pid = getpid();
    if (pp_os_tid != NULL)
        *pp_os_tid = SB_Thread::Sthr::self_id();
    if (pp_compid != NULL)
        *pp_compid = gv_ms_su_compid;
    if (pp_pnid != NULL)
        *pp_pnid = gv_ms_su_pnid;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");
    return XZFIL_ERR_OK;
}

#ifdef SQ_PHANDLE_VERIFIER
//
// Purpose: get my info
//
SB_Export int msg_mon_get_my_info4(int           *pp_mon_nid,
                                   int           *pp_mon_pid,
                                   char          *pp_mon_name,
                                   int            pv_mon_name_len,
                                   int           *pp_mon_ptype,
                                   int           *pp_mon_zid,
                                   int           *pp_os_pid,
                                   long          *pp_os_tid,
                                   int           *pp_compid,
                                   int           *pp_pnid,
                                   SB_Verif_Type *pp_mon_verifier) {
    const char *WHERE = "msg_mon_get_my_info4";
    SB_API_CTR (lv_zctr, MSG_MON_GET_MY_INFO4);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER m-id=%p/%p, m-name=%p, m-name-len=%d, m-ptype=%p, m-zid=%p, os-pid=%p, os-tid=%p, compid=%p, pnid=%p, m-id-verifier=%p\n",
                           pfp(pp_mon_nid),
                           pfp(pp_mon_pid),
                           pp_mon_name,
                           pv_mon_name_len,
                           pfp(pp_mon_ptype),
                           pfp(pp_mon_zid),
                           pfp(pp_os_pid),
                           pfp(pp_os_tid),
                           pfp(pp_compid),
                           pfp(pp_pnid),
                           pfp(pp_mon_verifier));

    //
    // do not check msg_init [in case logger calls]
    //

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "m-id=%d/%d, m-name=%s, m-ptype=%d, m-zid=%d, os-pid=%d, os-tid=%d, compid=%d, pnid=%d, m-verifier=" PFVY "\n",
                           gv_ms_su_nid,
                           gv_ms_su_pid,
                           ga_ms_su_pname,
                           gv_ms_su_ptype,
                           gv_ms_su_zid,
                           getpid(),
                           static_cast<int>(SB_Thread::Sthr::self_id()),
                           gv_ms_su_compid,
                           gv_ms_su_pnid,
                           gv_ms_su_verif);
    if (pp_mon_nid != NULL)
        *pp_mon_nid = gv_ms_su_nid;
    if (pp_mon_pid != NULL)
        *pp_mon_pid = gv_ms_su_pid;
    if ((pp_mon_name != NULL) && (pv_mon_name_len > 1)) {
        strncpy(pp_mon_name, ga_ms_su_pname, pv_mon_name_len);
        pp_mon_name[pv_mon_name_len-1] = '\0';
    }
    if (pp_mon_ptype != NULL)
        *pp_mon_ptype = gv_ms_su_ptype;
    if (pp_mon_zid != NULL)
        *pp_mon_zid = gv_ms_su_zid;
    if (pp_os_pid != NULL)
        *pp_os_pid = getpid();
    if (pp_os_tid != NULL)
        *pp_os_tid = SB_Thread::Sthr::self_id();
    if (pp_compid != NULL)
        *pp_compid = gv_ms_su_compid;
    if (pp_pnid != NULL)
        *pp_pnid = gv_ms_su_pnid;
    if (pp_mon_verifier != NULL)
        *pp_mon_verifier = gv_ms_su_verif;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");
    return XZFIL_ERR_OK;
}
#endif

//
// Purpose: get my process name
//
SB_Export int msg_mon_get_my_process_name(char *pp_name, int pv_len) {
    const char *WHERE = "msg_mon_get_my_process_name";
    SB_API_CTR (lv_zctr, MSG_MON_GET_MY_PROCESS_NAME);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=%p, len=%d\n",
                           pp_name, pv_len);
    if (!gv_ms_calls_ok) // msg_mon_get_my_process_name
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (static_cast<int>((strlen(ga_ms_su_pname) + 1)) > pv_len)
        return XZFIL_ERR_BOUNDSERR;
    strcpy(pp_name, ga_ms_su_pname);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK name=%s\n", pp_name);
    return XZFIL_ERR_OK;
}

//
// Purpose: get my segment-id
//
SB_Export int msg_mon_get_my_segid(int *pp_segid) {
    const char   *WHERE = "msg_mon_get_my_segid";
    char         *lp_p;
    SB_Buf_Lline  lv_line;
    int           lv_segid;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_MY_SEGID);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER segid=%p (monitor-port=%s)\n",
                           pfp(pp_segid), ga_ms_su_c_port);
    if (!gv_ms_calls_ok) // msg_mon_get_my_segid
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    lp_p = strstr(ga_ms_su_c_port, "$port#");
    if (lp_p == NULL) {
        lp_p = strchr(ga_ms_su_c_port, ':');
        if (lp_p == NULL) {
            sprintf(&lv_line, "monitor-port=%s does not contain '$port#' or ':'",
                    ga_ms_su_c_port);
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "%s\n", &lv_line);
            return ms_err_rtn_msg(WHERE, &lv_line, XZFIL_ERR_INVALIDSTATE);
        }
        lv_segid = atoi(&lp_p[1]);
    }
    else {
        lv_segid = atoi(&lp_p[6]);
    }
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK segid=%d\n", lv_segid);
    *pp_segid = lv_segid;
    return XZFIL_ERR_OK;
}

//
// Purpose: get node info (all nodes)
//
SB_Export int msg_mon_get_node_info(int                         *pp_count,
                                    int                          pv_max,
                                    MS_Mon_Node_Info_Entry_Type *pp_info) {
    const char   *WHERE = "msg_mon_get_node_info";
    Mon_Msg_Type *lp_msg;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_NODE_INFO);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_NODE_INFO, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER count=%d, max=%d, info=%p\n",
                           *pp_count, pv_max, pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_node_info
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_node_info(WHERE,
                                          lp_msg,
                                          lv_msg.get_error(),
                                          -1,
                                          false);
    int lv_cluster_nodes = lp_msg->u.reply.u.node_info.num_nodes;
    int lv_nodes = lp_msg->u.reply.u.node_info.num_returned;
    *pp_count = 0;
    if (lv_fserr == XZFIL_ERR_OK) {
        if (pp_info == NULL) {
            // return the number of combined logical and spare nodes
            *pp_count = (lp_msg->u.reply.u.node_info.num_nodes +
                         lp_msg->u.reply.u.node_info.num_spares);
        } else if (lv_nodes > pv_max)
            lv_fserr = XZFIL_ERR_BOUNDSERR;
        else {
            // copy results for user
            memcpy(pp_info,
                   lp_msg->u.reply.u.node_info.node,
                   lv_nodes * sizeof(MS_Mon_Node_Info_Entry_Type));
        }
    }
    if ((lv_fserr == XZFIL_ERR_OK) && (pp_info != NULL)) {
        int lv_total_nodes = lv_nodes;
        while (lv_total_nodes < lv_cluster_nodes) {
            int lv_fserr = msg_mon_send_node_info(WHERE,
                                                  lp_msg,
                                                  lv_msg.get_error(),
                                                  -1,
                                                  true);
            if (lv_fserr != XZFIL_ERR_OK)
                break;
            lv_nodes = lp_msg->u.reply.u.node_info.num_returned;
            if (lv_nodes == 0)
                break;
            if (pp_info == NULL) {
            } else if ((lv_total_nodes + lv_nodes) > pv_max) {
                lv_fserr = XZFIL_ERR_BOUNDSERR;
                break;
            } else {
                // copy results for user
                memcpy(&pp_info[lv_total_nodes],
                       lp_msg->u.reply.u.node_info.node,
                       lv_nodes * sizeof(MS_Mon_Node_Info_Entry_Type));
            }
            lv_total_nodes += lv_nodes;
        }
        if (lv_fserr == XZFIL_ERR_OK)
            *pp_count = lv_total_nodes;
    }
    if (gv_ms_trace_mon) {
        trace_where_printf(WHERE, "count=%d\n", *pp_count);
        if (lv_fserr == XZFIL_ERR_OK)
            trace_where_printf(WHERE, "EXIT OK\n");
        else
            trace_where_printf(WHERE, "EXIT FAILURE fserr=%d\n", lv_fserr);
    }
    return lv_fserr;
}

//
// Purpose: get node info (all nodes)
//
SB_Export int msg_mon_get_node_info2(int                         *pp_count,
                                     int                          pv_max,
                                     MS_Mon_Node_Info_Entry_Type *pp_info,
                                     int                         *pp_node_count,
                                     int                         *pp_pnode_count,
                                     int                         *pp_spares_count,
                                     int                         *pp_available_spares_count) {
    const char   *WHERE = "msg_mon_get_node_info2";
    Mon_Msg_Type *lp_msg;
    int           lv_available_spares_count;
    int           lv_node_count;
    int           lv_pnode_count;
    int           lv_spares_count;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_NODE_INFO2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_NODE_INFO2, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER count=%d, max=%d, info=%p, node-count=%p, pnode-count=%p, spares-count=%p, available-spares-count=%p\n",
                           *pp_count, pv_max, pfp(pp_info), pfp(pp_node_count), pfp(pp_pnode_count), pfp(pp_spares_count), pfp(pp_available_spares_count));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_node_info2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_node_info(WHERE,
                                          lp_msg,
                                          lv_msg.get_error(),
                                          -1,
                                          false);
    int lv_cluster_nodes = lp_msg->u.reply.u.node_info.num_nodes;
    int lv_nodes = lp_msg->u.reply.u.node_info.num_returned;
    *pp_count = 0;
    if (lv_fserr == XZFIL_ERR_OK) {
        lv_node_count = lp_msg->u.reply.u.node_info.num_nodes;
        lv_pnode_count = lp_msg->u.reply.u.node_info.num_pnodes;
        lv_spares_count = lp_msg->u.reply.u.node_info.num_spares;
        lv_available_spares_count = lp_msg->u.reply.u.node_info.num_available_spares;
        if (pp_info == NULL) {
            // return the number of combined logical and spare nodes
            *pp_count = (lp_msg->u.reply.u.node_info.num_nodes +
                         lp_msg->u.reply.u.node_info.num_spares);
            if (pp_node_count != NULL)
                *pp_node_count = lv_node_count;
            if (pp_pnode_count != NULL)
                *pp_pnode_count = lv_pnode_count;
            if (pp_spares_count != NULL)
                *pp_spares_count = lv_spares_count;
            if (pp_available_spares_count != NULL)
                *pp_available_spares_count = lv_available_spares_count;
        } else if (lv_nodes > pv_max)
            lv_fserr = XZFIL_ERR_BOUNDSERR;
        else {
            // copy results for user
            memcpy(pp_info,
                   lp_msg->u.reply.u.node_info.node,
                   lv_nodes * sizeof(MS_Mon_Node_Info_Entry_Type));
        }
    } else {
        lv_node_count = 0;
        lv_pnode_count = 0;
        lv_spares_count = 0;
        lv_available_spares_count = 0;
    }
    if ((lv_fserr == XZFIL_ERR_OK) && (pp_info != NULL)) {
        int lv_total_nodes = lv_nodes;
        while (lv_total_nodes < lv_cluster_nodes) {
            int lv_fserr = msg_mon_send_node_info(WHERE,
                                                  lp_msg,
                                                  lv_msg.get_error(),
                                                  -1,
                                                  true);
            if (lv_fserr != XZFIL_ERR_OK)
                break;
            lv_nodes = lp_msg->u.reply.u.node_info.num_returned;
            if (lv_nodes == 0)
                break;
            if (pp_info == NULL) {
            } else if ((lv_total_nodes + lv_nodes) > pv_max) {
                lv_fserr = XZFIL_ERR_BOUNDSERR;
                break;
            } else {
                // copy results for user
                memcpy(&pp_info[lv_total_nodes],
                       lp_msg->u.reply.u.node_info.node,
                       lv_nodes * sizeof(MS_Mon_Node_Info_Entry_Type));
            }
            lv_total_nodes += lv_nodes;
        }
        if (lv_fserr == XZFIL_ERR_OK)
            *pp_count = lv_total_nodes;
    }
    if (gv_ms_trace_mon) {
        trace_where_printf(WHERE, "count=%d, node-count=%d, pnode-count=%d, spares-count=%d, available-spares-count=%d\n", *pp_count, lv_node_count, lv_pnode_count, lv_spares_count, lv_available_spares_count);
        if (lv_fserr == XZFIL_ERR_OK)
            trace_where_printf(WHERE, "EXIT OK\n");
        else
            trace_where_printf(WHERE, "EXIT FAILURE fserr=%d\n", lv_fserr);
    }
    return lv_fserr;
}

//
// Purpose: get node info (all nodes)
//
SB_Export int msg_mon_get_node_info_all(MS_Mon_Node_Info_Type *pp_info) {
    const char   *WHERE = "msg_mon_get_node_info_all";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    int           lv_nid;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_NODE_INFO_DETAIL);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_NODE_INFO_DETAIL, 0);

    lv_nid = -1;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER info=%p\n", pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_node_info_all
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_NodeInfo;
    lp_msg->u.request.u.node_info.nid = gv_ms_su_nid;
    lp_msg->u.request.u.node_info.pid = gv_ms_su_pid;
    lp_msg->u.request.u.node_info.target_nid = lv_nid;
    lp_msg->u.request.u.node_info.last_nid = -1;
    lp_msg->u.request.u.node_info.last_pnid = -1;
    lp_msg->u.request.u.node_info.continuation = false;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send node-info req to mon, p-id=%d/%d, t-nid=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid, lv_nid);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "node-info",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "node-info req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_NodeInfo)) {
        struct NodeInfo_reply_def *lp_info = &lp_msg->u.reply.u.node_info;
        lv_mpierr = lp_info->return_code;
        int lv_cluster_nodes = lp_info->num_nodes;
        int lv_nodes = lp_info->num_returned;
        if (gv_ms_trace_mon) {
            if (lv_mpierr == MPI_SUCCESS) {
                trace_where_printf(WHERE, "EXIT OK node-info req, cluster-nodes=%d, returned-nodes=%d\n",
                                   lv_cluster_nodes, lv_nodes);
                for (int lv_node = 0; lv_node < lv_nodes; lv_node++) {
                    trace_where_printf(WHERE, "EXIT OK node-info req[%d], nid=%d, state=%d, cpus=%d, procs=%d\n",
                                       lv_node,
                                       lp_info->node[lv_node].nid,
                                       lp_info->node[lv_node].state,
                                       lp_info->node[lv_node].processors,
                                       lp_info->node[lv_node].process_count);
                    trace_where_printf(WHERE, "EXIT OK node-info req[%d], mem-free=%d, swap-free=%d, cache-free=%d\n",
                                       lv_node,
                                       lp_info->node[lv_node].memory_free,
                                       lp_info->node[lv_node].swap_free,
                                       lp_info->node[lv_node].cache_free);
                }
            } else
                trace_where_printf(WHERE, "EXIT FAILED node-info req, ret=%d\n",
                                   lv_mpierr);
        }
        // copy results for user
        memcpy(pp_info, lp_info, sizeof(MS_Mon_Node_Info_Type));
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: get node info (specific node)
//
SB_Export int msg_mon_get_node_info_detail(int                    pv_nid,
                                           MS_Mon_Node_Info_Type *pp_info) {
    const char   *WHERE = "msg_mon_get_node_info_detail";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_NODE_INFO_DETAIL);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_NODE_INFO_DETAIL, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER nid=%d, info=%p\n",
                           pv_nid, pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_node_info_detail
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_NodeInfo;
    lp_msg->u.request.u.node_info.nid = gv_ms_su_nid;
    lp_msg->u.request.u.node_info.pid = gv_ms_su_pid;
    lp_msg->u.request.u.node_info.target_nid = pv_nid;
    lp_msg->u.request.u.node_info.last_nid = -1;
    lp_msg->u.request.u.node_info.last_pnid = -1;
    lp_msg->u.request.u.node_info.continuation = false;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send node-info req to mon, p-id=%d/%d, t-nid=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid, pv_nid);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "node-info",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "node-info req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_NodeInfo)) {
        struct NodeInfo_reply_def *lp_info = &lp_msg->u.reply.u.node_info;
        lv_mpierr = lp_info->return_code;
        int lv_cluster_nodes = lp_info->num_nodes;
        int lv_nodes = lp_info->num_returned;
        if (gv_ms_trace_mon) {
            if (lv_mpierr == MPI_SUCCESS) {
                trace_where_printf(WHERE, "EXIT OK node-info req, cluster-nodes=%d, returned-nodes=%d\n",
                                   lv_cluster_nodes, lv_nodes);
                for (int lv_node = 0; lv_node < lv_nodes; lv_node++) {
                    trace_where_printf(WHERE, "EXIT OK node-info req[%d], nid=%d, state=%d, cpus=%d, procs=%d\n",
                                       lv_node,
                                       lp_info->node[lv_node].nid,
                                       lp_info->node[lv_node].state,
                                       lp_info->node[lv_node].processors,
                                       lp_info->node[lv_node].process_count);
                    trace_where_printf(WHERE, "EXIT OK node-info req[%d], mem-free=%d, swap-free=%d, cache-free=%d\n",
                                       lv_node,
                                       lp_info->node[lv_node].memory_free,
                                       lp_info->node[lv_node].swap_free,
                                       lp_info->node[lv_node].cache_free);
                }
            } else
                trace_where_printf(WHERE, "EXIT FAILED node-info req, ret=%d\n",
                                   lv_mpierr);
        }
        // copy results for user
        memcpy(pp_info, lp_info, sizeof(MS_Mon_Node_Info_Type));
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: get open info
//
SB_Export int msg_mon_get_open_info(int                    pv_nid,
                                    int                    pv_pid,
                                    char                  *pp_name,
                                    int                    pv_opened,
                                    MS_Mon_Open_Info_Type *pp_info) {
    const char   *WHERE = "msg_mon_get_open_info";
    SB_API_CTR   (lv_zctr, MSG_MON_GET_OPEN_INFO);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_OPEN_INFO, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER p-id=%d/%d, pname=%s, opened=%d, info=%p\n",
                           pv_nid, pv_pid, pp_name, pv_opened, pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_open_info
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    // This routine currently does not support the return of process open state.
    // ZFIL_ERR_INVALOP is always returned.

    return ms_err_rtn_msg(WHERE, "msg_mon_get_open_info is invalid",
                          XZFIL_ERR_INVALOP);
}

// Purpose: get open info (max specified)
//
SB_Export int msg_mon_get_open_info_max(int                        pv_nid,
                                        int                        pv_pid,
                                        char                      *pp_name,
                                        int                        pv_opened,
                                        int                       *pp_count,
                                        int                        pv_max,
                                        MS_Mon_Open_Info_Max_Type *pp_info) {
    const char          *WHERE = "msg_mon_get_open_info_max";
    SB_API_CTR          (lv_zctr, MSG_MON_GET_OPEN_INFO_MAX);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_OPEN_INFO_MAX, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER p-id=%d/%d, pname=%s, opened=%d, count=%p, max=%d, info=%p\n",
                           pv_nid, pv_pid, pp_name, pv_opened, pfp(pp_count), pv_max, pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_open_info_max
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    // This routine currently does not support the return of process open state.
    // ZFIL_ERR_INVALOP is always returned.

    return ms_err_rtn_msg(WHERE, "msg_mon_get_open_info is invalid",
                          XZFIL_ERR_INVALOP);

}

//
// Purpose: get process info
// if pp_name if NULL, empty, or self, return self's nid/pid
//
SB_Export int msg_mon_get_process_info(char *pp_name,
                                       int  *pp_nid,
                                       int  *pp_pid) {
    const char   *WHERE = "msg_mon_get_process_info";
    Mon_Msg_Type *lp_msg;
    SB_API_CTR (lv_zctr, MSG_MON_GET_PROCESS_INFO);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_PROCESS_INFO, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=%s, nid=%p, pid=%p\n",
                           pp_name, pfp(pp_nid), pfp(pp_pid));
    if (!gv_ms_calls_ok) // msg_mon_get_process_info (self)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if ((pp_name == NULL) ||
        (strlen(pp_name) == 0) ||
        (strcasecmp(pp_name, ga_ms_su_pname) == 0)) {
        if (pp_nid != NULL)
            *pp_nid = gv_ms_su_nid;
        if (pp_pid != NULL)
            *pp_pid = gv_ms_su_pid;
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXIT OK (self) p-id=%d/%d\n",
                               gv_ms_su_nid, gv_ms_su_pid);
        return XZFIL_ERR_OK;
    }

    if (!gv_ms_mon_calls_ok) // msg_mon_get_process_info
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_process_info(WHERE,
                                             lp_msg,
                                             lv_msg.get_error(),
                                             -1,
                                             -1,
#ifdef SQ_PHANDLE_VERIFIER
                                             -1,
#endif
                                             pp_name,
                                             ProcessType_Undefined,
                                             false);
    int lv_procs = lp_msg->u.reply.u.process_info.num_processes;
    int lv_nid = lp_msg->u.reply.u.process_info.process[0].nid;
    int lv_pid = lp_msg->u.reply.u.process_info.process[0].pid;
    if (lv_fserr == XZFIL_ERR_OK) {
        if (lv_procs < 1)
            lv_fserr = XZFIL_ERR_NOSUCHDEV;
        else {
            if (pp_nid != NULL)
                *pp_nid = lv_nid;
            if (pp_pid != NULL)
                *pp_pid = lv_pid;
        }
    }
    if (gv_ms_trace_mon) {
        if (lv_fserr == XZFIL_ERR_OK)
            trace_where_printf(WHERE, "EXIT OK process-info req, procs=%d, p-id=%d/%d\n",
                               lv_procs, lv_nid, lv_pid);
        else
            trace_where_printf(WHERE, "EXIT FAILURE fserr=%d\n", lv_fserr);
    }
    return lv_fserr;
}

#ifdef SQ_PHANDLE_VERIFIER
//
// Purpose: get process info
// if pp_name if NULL, empty, or self, return self's nid/pid/verifier
//
SB_Export int msg_mon_get_process_info2(char          *pp_name,
                                        int           *pp_nid,
                                        int           *pp_pid,
                                        SB_Verif_Type *pp_verif) {
    const char   *WHERE = "msg_mon_get_process_info2";
    Mon_Msg_Type *lp_msg;
    SB_API_CTR (lv_zctr, MSG_MON_GET_PROCESS_INFO2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_PROCESS_INFO2, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=%s, nid=%p, pid=%p, verifier=%p\n",
                           pp_name, pfp(pp_nid), pfp(pp_pid), pfp(pp_verif));
    if (!gv_ms_calls_ok) // msg_mon_get_process_info (self)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if ((pp_name == NULL) ||
        (strlen(pp_name) == 0) ||
        (strcasecmp(pp_name, ga_ms_su_pname) == 0)) {
        if (pp_nid != NULL)
            *pp_nid = gv_ms_su_nid;
        if (pp_pid != NULL)
            *pp_pid = gv_ms_su_pid;
        if (pp_verif != NULL)
            *pp_verif = gv_ms_su_verif;
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXIT OK (self) p-id=%d/%d/" PFVY "\n",
                               gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif);
        return XZFIL_ERR_OK;
    }

    if (!gv_ms_mon_calls_ok) // msg_mon_get_process_info
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_process_info(WHERE,
                                             lp_msg,
                                             lv_msg.get_error(),
                                             -1,
                                             -1,
#ifdef SQ_PHANDLE_VERIFIER
                                             -1,
#endif
                                             pp_name,
                                             ProcessType_Undefined,
                                             false);
    int lv_procs = lp_msg->u.reply.u.process_info.num_processes;
    int lv_nid = lp_msg->u.reply.u.process_info.process[0].nid;
    int lv_pid = lp_msg->u.reply.u.process_info.process[0].pid;
    SB_Verif_Type lv_verif = lp_msg->u.reply.u.process_info.process[0].verifier;
    if (lv_fserr == XZFIL_ERR_OK) {
        if (lv_procs < 1)
            lv_fserr = XZFIL_ERR_NOSUCHDEV;
        else {
            if (pp_nid != NULL)
                *pp_nid = lv_nid;
            if (pp_pid != NULL)
                *pp_pid = lv_pid;
            if (pp_verif != NULL)
                *pp_verif = lv_verif;
        }
    }
    if (gv_ms_trace_mon) {
        if (lv_fserr == XZFIL_ERR_OK)
            trace_where_printf(WHERE, "EXIT OK process-info req, procs=%d, p-id=%d/%d/" PFVY "\n",
                               lv_procs, lv_nid, lv_pid, lv_verif);
        else
            trace_where_printf(WHERE, "EXIT FAILURE fserr=%d\n", lv_fserr);
    }
    return lv_fserr;
}
#endif

//
// Purpose: get process info
// if pp_name if NULL, empty, or self, return self's nid/pid
//
SB_Export int msg_mon_get_process_info_detail(char                     *pp_name,
                                              MS_Mon_Process_Info_Type *pp_info) {
    const char    *WHERE = "msg_mon_get_process_info_detail";
    Mon_Msg_Type  *lp_msg;
    int            lv_nid;
    int            lv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type  lv_verif;
#endif
    SB_API_CTR    (lv_zctr, MSG_MON_GET_PROCESS_INFO_DETAIL);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_PROCESS_INFO_DETAIL, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=%s, info=%p\n",
                           pp_name, pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_process_info_detail
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if ((pp_name == NULL) || (strlen(pp_name) == 0)) {
        pp_name = ga_ms_su_pname;
        lv_nid = gv_ms_su_nid;
        lv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lv_verif = -1;
#endif
    } else if (strcasecmp(pp_name, ga_ms_su_pname) == 0) {
        lv_nid = gv_ms_su_nid;
        lv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lv_verif = -1;
#endif
    } else {
        lv_nid = -1;
        lv_pid = -1;
#ifdef SQ_PHANDLE_VERIFIER
        lv_verif = -1;
#endif
    }

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_process_info(WHERE,
                                             lp_msg,
                                             lv_msg.get_error(),
                                             lv_nid,
                                             lv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                             lv_verif,
#endif
                                             pp_name,
                                             ProcessType_Undefined,
                                             false);
    int lv_procs = lp_msg->u.reply.u.process_info.num_processes;
    if (lv_fserr == XZFIL_ERR_OK) {
        if (lv_procs < 1)
            lv_fserr = XZFIL_ERR_NOSUCHDEV;
        else {
            // copy results for user
            memcpy(pp_info,
                   lp_msg->u.reply.u.process_info.process,
                   sizeof(MS_Mon_Process_Info_Type));
        }
    }
    if (gv_ms_trace_mon) {
        if (lv_fserr == XZFIL_ERR_OK)
            trace_where_printf(WHERE, "EXIT OK\n");
        else
            trace_where_printf(WHERE, "EXIT FAILURE fserr=%d\n", lv_fserr);
    }
    return lv_fserr;
}

//
// Purpose: get process info for type
// If pp_info is NULL - don't return info [to get count]
//
SB_Export int msg_mon_get_process_info_type(int                       pv_ptype,
                                            int                      *pp_count,
                                            int                       pv_max,
                                            MS_Mon_Process_Info_Type *pp_info) {
    const char   *WHERE = "msg_mon_get_process_info_type";
    Mon_Msg_Type *lp_msg;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_PROCESS_INFO_TYPE);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_PROCESS_INFO_TYPE, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER type=%d, count=%p, max=%d, info=%p\n",
                           pv_ptype, pfp(pp_count), pv_max, pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_process_info_type
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_process_info(WHERE,
                                             lp_msg,
                                             lv_msg.get_error(),
                                             -1,
                                             -1,
#ifdef SQ_PHANDLE_VERIFIER
                                             -1,
#endif
                                             NULL,
                                             pv_ptype,
                                             false);
    int lv_procs = lp_msg->u.reply.u.process_info.num_processes;
    *pp_count = 0;
    if (lv_fserr == XZFIL_ERR_OK) {
        if (pp_info == NULL) {
        } else if (lv_procs > pv_max)
            lv_fserr = XZFIL_ERR_BOUNDSERR;
        else {
            // copy results for user
            memcpy(pp_info,
                   lp_msg->u.reply.u.process_info.process,
                   lv_procs * sizeof(MS_Mon_Process_Info_Type));
        }
    }
    if (lv_fserr == XZFIL_ERR_OK) {
        int lv_total_procs = lv_procs;
        while (lp_msg->u.reply.u.process_info.more_data) {
            lv_fserr = msg_mon_send_process_info(WHERE,
                                                 lp_msg,
                                                 lv_msg.get_error(),
                                                 -1,
                                                 -1,
#ifdef SQ_PHANDLE_VERIFIER
                                                 -1,
#endif
                                                 NULL,
                                                 pv_ptype,
                                                 true);
            if (lv_fserr != XZFIL_ERR_OK)
                break;
            lv_procs = lp_msg->u.reply.u.process_info.num_processes;
            if (lv_procs == 0)
                break;
            if (pp_info == NULL) {
            } else if ((lv_total_procs + lv_procs) > pv_max) {
                lv_fserr = XZFIL_ERR_BOUNDSERR;
                break;
            } else {
                // copy results for user
                memcpy(&pp_info[lv_total_procs],
                       lp_msg->u.reply.u.process_info.process,
                       lv_procs * sizeof(MS_Mon_Process_Info_Type));
            }
            lv_total_procs += lv_procs;
        }
        if (lv_fserr == XZFIL_ERR_OK)
            *pp_count = lv_total_procs;
    }
    if (gv_ms_trace_mon) {
        trace_where_printf(WHERE, "EXIT count=%d\n", *pp_count);
        if (lv_fserr == XZFIL_ERR_OK)
            trace_where_printf(WHERE, "EXIT OK\n");
        else
            trace_where_printf(WHERE, "EXIT FAILURE fserr=%d\n", lv_fserr);
    }
    return lv_fserr;
}

//
// Purpose: get process name
//
SB_Export int msg_mon_get_process_name(int   pv_nid,
                                       int   pv_pid,
                                       char *pp_name) {
    const char   *WHERE = "msg_mon_get_process_name";
    Mon_Msg_Type *lp_msg;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_PROCESS_NAME);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_PROCESS_NAME, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER p-id=%d/%d, pname=%p\n",
                           pv_nid, pv_pid, pp_name);
    if (!gv_ms_mon_calls_ok) // msg_mon_get_process_name
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (gv_ms_shutdown_called)
        return ms_err_rtn_msg(WHERE, "shutdown called - cannot process",
                              XZFIL_ERR_INVALIDSTATE);
    if (pv_nid < 0)
        return ms_err_rtn_msg(WHERE, "invalid nid (<0)", XZFIL_ERR_BOUNDSERR);
    if (pv_pid < 0)
        return ms_err_rtn_msg(WHERE, "invalid pid (<0)", XZFIL_ERR_BOUNDSERR);
    if (pp_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid name (null)",
                              XZFIL_ERR_BOUNDSERR);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_process_info(WHERE,
                                             lp_msg,
                                             lv_msg.get_error(),
                                             pv_nid,
                                             pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                             -1,
#endif
                                             NULL,
                                             ProcessType_Undefined,
                                             false);
    if (lv_fserr == XZFIL_ERR_OK) {
        int lv_procs = lp_msg->u.reply.u.process_info.num_processes;
        SB_util_assert_ile(lv_procs, 1); // sw fault
        if (lv_procs == 0)
            return ms_err_rtn_msg(WHERE, "no process returned", XZFIL_ERR_NOSUCHDEV);
        else
            strcpy(pp_name,
                   lp_msg->u.reply.u.process_info.process[0].process_name);
    }
    return lv_fserr;
}

#ifdef SQ_PHANDLE_VERIFIER
//
// Purpose: get process name
//
SB_Export int msg_mon_get_process_name2(int            pv_nid,
                                        int            pv_pid,
                                        SB_Verif_Type  pv_verif,
                                        char          *pp_name) {
    const char   *WHERE = "msg_mon_get_process_name";
    Mon_Msg_Type *lp_msg;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_PROCESS_NAME2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_PROCESS_NAME2, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER p-id=%d/%d/" PFVY ", pname=%p\n",
                           pv_nid, pv_pid, pv_verif, pp_name);
    if (!gv_ms_mon_calls_ok) // msg_mon_get_process_name
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (gv_ms_shutdown_called)
        return ms_err_rtn_msg(WHERE, "shutdown called - cannot process",
                              XZFIL_ERR_INVALIDSTATE);
    if (pv_nid < 0)
        return ms_err_rtn_msg(WHERE, "invalid nid (<0)", XZFIL_ERR_BOUNDSERR);
    if (pv_pid < 0)
        return ms_err_rtn_msg(WHERE, "invalid pid (<0)", XZFIL_ERR_BOUNDSERR);
    if (pp_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid name (null)",
                              XZFIL_ERR_BOUNDSERR);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_process_info(WHERE,
                                             lp_msg,
                                             lv_msg.get_error(),
                                             pv_nid,
                                             pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                             pv_verif,
#endif
                                             NULL,
                                             ProcessType_Undefined,
                                             false);
    if (lv_fserr == XZFIL_ERR_OK) {
        int lv_procs = lp_msg->u.reply.u.process_info.num_processes;
        SB_util_assert_ile(lv_procs, 1); // sw fault
        if (lv_procs == 0)
            return ms_err_rtn_msg(WHERE, "no process returned", XZFIL_ERR_NOSUCHDEV);
        else {
            if (pv_verif < 0) {
                strcpy(pp_name,
                       lp_msg->u.reply.u.process_info.process[0].process_name);
            } else if (lp_msg->u.reply.u.process_info.process[0].verifier == pv_verif) {
                strcpy(pp_name,
                       lp_msg->u.reply.u.process_info.process[0].process_name);
            } else {
                return ms_err_rtn_msg(WHERE, "verifier mismatch", XZFIL_ERR_NOSUCHDEV);
            }
        }
    }
    return lv_fserr;
}
#endif
//
// Purpose: get reference count for phandle
//
int msg_mon_get_ref_count(SB_Phandle_Type *pp_phandle) {
    int lv_ref_count;

    char *lp_name = ms_od_map_phandle_to_name(pp_phandle);
    Ms_Od_Type *lp_od = ms_od_map_name_to_od(lp_name);
    if (lp_od != NULL)
        lv_ref_count = lp_od->iv_ref_count;
    else
        lv_ref_count = 0;
    return lv_ref_count;
}


//
// Purpose: get zone info (all nodes)
//
SB_Export int msg_mon_get_zone_info(int                         *pp_count,
                                    int                          pv_max,
                                    MS_Mon_Zone_Info_Entry_Type *pp_info) {
    const char   *WHERE = "msg_mon_get_zone_info";
    Mon_Msg_Type *lp_msg;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_ZONE_INFO);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_ZONE_INFO, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER count=%p, max=%d, info=%p\n",
                           pfp(pp_count), pv_max, pfp(pp_info));

    if (!gv_ms_mon_calls_ok) // msg_mon_get_zone_info
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    int lv_fserr = msg_mon_send_zone_info(WHERE,
                                          lp_msg,
                                          lv_msg.get_error(),
                                          -1,
                                          -1,
                                          false);

    // num_nodes is the number of logical nodes in the cluster
    // num_returned is the number of node entries returned

    int lv_cluster_nodes = lp_msg->u.reply.u.zone_info.num_nodes;
    int lv_nodes = lp_msg->u.reply.u.zone_info.num_returned;
    *pp_count = 0;
    if (lv_fserr == XZFIL_ERR_OK) {
        if (pp_info == NULL) {
            // return the number of nodes returned
            *pp_count = lp_msg->u.reply.u.zone_info.num_returned;
        } else if (lv_nodes > pv_max)
            lv_fserr = XZFIL_ERR_BOUNDSERR;
        else {
            // copy results for user
            memcpy(pp_info,
                   lp_msg->u.reply.u.zone_info.node,
                   lv_nodes * sizeof(MS_Mon_Zone_Info_Entry_Type));
        }
    }
    if ((lv_fserr == XZFIL_ERR_OK) && (pp_info != NULL)) {
        int lv_total_nodes = lv_nodes;
        while (lv_total_nodes < lv_cluster_nodes) {
            int lv_fserr = msg_mon_send_zone_info(WHERE,
                                                  lp_msg,
                                                  lv_msg.get_error(),
                                                  -1,
                                                  -1,
                                                  true);
            if (lv_fserr != XZFIL_ERR_OK)
                break;
            lv_nodes = lp_msg->u.reply.u.zone_info.num_returned;
            if (lv_nodes == 0)
                break;
            if (pp_info == NULL) {
            } else if ((lv_total_nodes + lv_nodes) > pv_max) {
                lv_fserr = XZFIL_ERR_BOUNDSERR;
                break;
            } else {
                // copy results for user
                memcpy(&pp_info[lv_total_nodes],
                       lp_msg->u.reply.u.zone_info.node,
                       lv_nodes * sizeof(MS_Mon_Zone_Info_Entry_Type));
            }
            lv_total_nodes += lv_nodes;
        }
        if (lv_fserr == XZFIL_ERR_OK)
            *pp_count = lv_total_nodes;
    }
    if (gv_ms_trace_mon) {
        trace_where_printf(WHERE, "count=%d\n", *pp_count);
        if (lv_fserr == XZFIL_ERR_OK)
            trace_where_printf(WHERE, "EXIT OK\n");
        else
            trace_where_printf(WHERE, "EXIT FAILURE fserr=%d\n", lv_fserr);
    }
    return lv_fserr;
}

//
// Purpose: get zone info (specific nid or zid)
//
SB_Export int msg_mon_get_zone_info_detail(int                    pv_nid,
                                           int                    pv_zid,
                                           MS_Mon_Zone_Info_Type *pp_info) {
    const char   *WHERE = "msg_mon_get_zone_info_detail";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_GET_ZONE_INFO_DETAIL);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_GET_ZONE_INFO_DETAIL, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER nid=%d, zid=%d, info=%p\n",
                           pv_nid, pv_zid, pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_get_zone_info_detail
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    // Use msg_mon_get_zone_info to obtain all the node and zone ids
    if ((pv_nid == -1) && (pv_zid == -1))
        return ms_err_rtn_msg(WHERE, "Use msg_mon_get_zone_info", XZFIL_ERR_BOUNDSERR);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_ZoneInfo;
    lp_msg->u.request.u.zone_info.nid = gv_ms_su_nid;
    lp_msg->u.request.u.zone_info.pid = gv_ms_su_pid;
    lp_msg->u.request.u.zone_info.target_nid = pv_nid;
    lp_msg->u.request.u.zone_info.target_zid = pv_zid;
    lp_msg->u.request.u.zone_info.last_nid = -1;
    lp_msg->u.request.u.zone_info.last_pnid = -1;
    lp_msg->u.request.u.zone_info.continuation = false;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send zone-info req to mon, p-id=%d/%d, t-nid=%d, t-zid=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid, pv_nid, pv_zid);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "zone-info",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "zone-info req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_ZoneInfo)) {
        struct ZoneInfo_reply_def *lp_info = &lp_msg->u.reply.u.zone_info;
        lv_mpierr = lp_info->return_code;
        int lv_cluster_nodes = lp_info->num_nodes;
        int lv_nodes = lp_info->num_returned;
        if (gv_ms_trace_mon) {
            if (lv_mpierr == MPI_SUCCESS) {
                trace_where_printf(WHERE, "EXIT OK zone-info req, cluster-nodes=%d, returned-nodes=%d\n",
                                   lv_cluster_nodes, lv_nodes);
                for (int lv_node = 0; lv_node < lv_nodes; lv_node++) {
                    trace_where_printf(WHERE, "EXIT OK node-info req[%d], nid=%d, zid= %d, state=%d, node name=%s\n",
                                       lv_node,
                                       lp_info->node[lv_node].nid,
                                       lp_info->node[lv_node].zid,
                                       lp_info->node[lv_node].pstate,
                                       lp_info->node[lv_node].node_name);
                }
            } else
                trace_where_printf(WHERE, "EXIT FAILED zone-info req, ret=%d\n",
                                   lv_mpierr);
        }
        // copy results for user
        memcpy(pp_info, lp_info, sizeof(MS_Mon_Zone_Info_Type));
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}
//
// Purpose: Deal with helper (callback-target)
//
void msg_mon_helper_cbt(MS_Md_Type *pp_md) {
    const char   *WHERE = "msg_mon_helper_cbt";
    Mon_Msg_Type *lp_msg;
    const char   *lp_msg_type;

    lp_msg = reinterpret_cast<Mon_Msg_Type *>(pp_md->out.ip_recv_data);
    if (gv_ms_trace_mon) {
        lp_msg_type = msg_util_get_msg_type(lp_msg->type);
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, msg.type=%d(%s)\n",
                           pp_md->iv_link.iv_id.i,
                           pfp(pp_md),
                           lp_msg->type,
                           lp_msg_type);
    }
    switch (lp_msg->type) {
    case MsgType_Close:
        msg_mon_recv_msg_close(lp_msg);
        break;
    case MsgType_ProcessDeath:
        msg_mon_recv_msg_process_death(lp_msg);
        break;
    default:
        break;
    }
    MS_BUF_MGR_FREE(pp_md->out.ip_recv_data);
    pp_md->out.ip_recv_data = NULL;
    SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "helper-callback");
}

void msg_mon_init() {
    Mon_Msg_Type *lp_msg = NULL;
    int           lv_max_ods;
    int           lv_status;

    // make sure monitor and seabed are in sync
    // Check literals (low and high) - will get sizeof error if mismatched
    SB_util_static_assert(static_cast<int>(MS_Mon_State_UnMounted) ==
                          static_cast<int>(State_UnMounted)); // sw fault
    SB_util_static_assert(static_cast<int>(MS_Mon_State_Mounted) ==
                          static_cast<int>(State_Mounted)); // sw fault

    SB_util_static_assert(static_cast<int>(MS_Mon_JoiningPhase_Unknown) ==
                          static_cast<int>(JoiningPhase_Unknown)); // sw fault
    SB_util_static_assert(static_cast<int>(MS_Mon_JoiningPhase_Invalid) ==
                          static_cast<int>(JoiningPhase_Invalid)); // sw fault

    SB_util_static_assert(static_cast<int>(MS_Mon_State_Unknown) ==
                          static_cast<int>(State_Unknown)); // sw fault
    SB_util_static_assert(static_cast<int>(MS_Mon_State_Takeover) ==
                          static_cast<int>(State_Takeover)); // sw fault

    SB_util_static_assert(static_cast<int>(MS_Mon_ShutdownLevel_Undefined) ==
                          static_cast<int>(ShutdownLevel_Undefined)); // sw fault
    SB_util_static_assert(static_cast<int>(MS_Mon_ShutdownLevel_Abrupt) ==
                          static_cast<int>(ShutdownLevel_Abrupt)); // sw fault

    SB_util_static_assert(static_cast<int>(MS_Mon_ZoneType_Undefined) ==
                          static_cast<int>(ZoneType_Undefined)); // sw fault
    SB_util_static_assert(static_cast<int>(MS_Mon_ZoneType_Storage) ==
                          static_cast<int>(ZoneType_Storage)); // sw fault

    SB_util_static_assert(static_cast<int>(MS_Mon_ConfigType_Undefined) ==
                          static_cast<int>(ConfigType_Undefined)); // sw fault
    SB_util_static_assert(static_cast<int>(MS_Mon_ConfigType_Process) ==
                          static_cast<int>(ConfigType_Process)); // sw fault

    SB_util_static_assert(static_cast<int>(MS_MsgType_Change) ==
                          static_cast<int>(MsgType_Change)); // sw fault

    SB_util_static_assert(static_cast<int>(MS_ReqType_Close) ==
                          static_cast<int>(ReqType_Close)); // sw fault
    SB_util_static_assert(static_cast<int>(MS_ReqType_ZoneInfo) ==
                          static_cast<int>(ReqType_ZoneInfo)); // sw fault

    SB_util_static_assert(static_cast<int>(MS_ProcessType_Undefined) ==
                          static_cast<int>(ProcessType_Undefined)); // sw fault
    SB_util_static_assert(static_cast<int>(MS_ProcessType_SMS) ==
                          static_cast<int>(ProcessType_SMS)); // sw fault

    // Check structs - will get sizeof error if mismatched
    SB_util_static_assert(sizeof(MS_Mon_Change_def) ==
                          sizeof(Change_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Close_def) ==
                          sizeof(Close_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_NewProcess_Notice_def) ==
                          sizeof(NewProcess_Notice_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_NodeAdded_def) ==
                          sizeof(NodeAdded_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_NodeChanged_def) ==
                          sizeof(NodeChanged_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_NodeDeleted_def) ==
                          sizeof(NodeDeleted_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_NodeDown_def) ==
                          sizeof(NodeDown_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_NodeJoining_def) ==
                          sizeof(NodeJoining_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_NodeQuiesce_def) ==
                          sizeof(NodeQuiesce_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_NodeUp_def) ==
                          sizeof(NodeUp_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Open_def) ==
                          sizeof(Open_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_ProcessDeath_def) ==
                          sizeof(ProcessDeath_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Shutdown_def) ==
                          sizeof(Shutdown_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_SpareUp_def) ==
                          sizeof(SpareUp_def)); // sw fault
    // More structs
    SB_util_static_assert(sizeof(MS_Mon_Monitor_Stats_Type) ==
                          sizeof(lp_msg->u.reply.u.mon_info)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Open_Info_Type) ==
                          sizeof(lp_msg->u.reply.u.open_info)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Open_Info_Max_Type) ==
                          sizeof(lp_msg->u.reply.u.open_info.opens[0])); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Node_Info_Type) ==
                          sizeof(NodeInfo_reply_def)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Node_Info_Entry_Type) ==
                          sizeof(lp_msg->u.reply.u.node_info.node[0])); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Process_Info_Type) ==
                          sizeof(lp_msg->u.reply.u.process_info.process)/MAX_PROCINFO_LIST); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Reg_Get_Type) ==
                          sizeof(lp_msg->u.reply.u.get)); // sw fault
    SB_util_static_assert(sizeof(MS_Mon_Zone_Info_Type) ==
                          sizeof(lp_msg->u.reply.u.zone_info)); // sw fault
    // Check struct offsets
    SB_util_static_assert(offsetof(MS_Mon_Msg, type) ==
                          offsetof(Mon_Msg_Type, type)); // sw fault
    SB_util_static_assert(offsetof(MS_Mon_Msg, _fill1) ==
                          offsetof(Mon_Msg_Type, noreply)); // sw fault
    SB_util_static_assert(offsetof(MS_Mon_Msg, _fill1) ==
                          offsetof(Mon_Msg_Type, noreply)); // sw fault
    SB_util_static_assert(offsetof(MS_Mon_Msg, _fill2) ==
                          offsetof(Mon_Msg_Type, reply_tag)); // sw fault
    SB_util_static_assert(offsetof(MS_Mon_Msg, reqtype) ==
                          offsetof(Mon_Msg_Type, u.request.type)); // sw fault
    SB_util_static_assert(offsetof(MS_Mon_Msg, u.change) ==
                          offsetof(Mon_Msg_Type, u.request.u.change)); // sw fault

    // initialize
    gv_ms_streams = SB_Trans::Trans_Stream::MAX_STREAMS_DEFAULT;
    ms_getenv_int(gp_ms_env_streams_min, &gv_ms_streams);
    if (gv_ms_streams < SB_Trans::Trans_Stream::MAX_STREAMS_DEFAULT)
        gv_ms_streams = SB_Trans::Trans_Stream::MAX_STREAMS_DEFAULT;
    else if (gv_ms_streams < 64)
        gv_ms_streams = 64; // if def < s < 64, round-up to 64
    else if (gv_ms_streams < 256)
        gv_ms_streams = 256; // if 64 < s < 256, round-up to 256
    else
        gv_ms_streams = (gv_ms_streams + 1023) & ~1023; // if s >= 256, round up to multiple of 1024
    ms_getenv_int(gp_ms_env_streams_max, &gv_ms_streams_max);
    gv_ms_max_openers = gv_ms_streams;
    gv_ms_od_mgr.alloc_entry(); // allocate the first one
    lv_max_ods = 0;
    ms_getenv_int(gp_ms_env_max_cap_ods, &lv_max_ods);
    if (lv_max_ods > 0)
        gv_ms_od_mgr.set_cap_max(lv_max_ods);
    lv_status = gv_ms_open_sem.init(0, MS_MAX_OPEN_OUT);
    SB_util_assert_ieq(lv_status, 0);

    // get reserved md
    if (gp_ms_rsvd_md == NULL) {
        SB_Trans::Msg_Mgr::get_md(&gp_ms_rsvd_md, // msg_mon_init
                                  NULL,
                                  NULL,
                                  true,           // send
                                  NULL,           // fserr
                                  "msg_mon_init (rsvd-md)",
                                  MD_STATE_RSVD_MD);
        SB_util_assert_pne(gp_ms_rsvd_md, NULL); // TODO: can't get md
    }
}

int msg_mon_init_attach(const char *pp_where,
                        char       *pp_name) {
    char          la_host_name[MPI_MAX_PROCESSOR_NAME];
    char          la_short_host_name[MPI_MAX_PROCESSOR_NAME];
    FILE         *lp_file;
    char         *lp_nid;
    char         *lp_nodes;
    int           lv_errno;
    SB_Buf_Lline  lv_file_name;
    int           lv_fserr = XZFIL_ERR_OK;
    int           lv_nid;

    if ((pp_name != NULL) && (strlen(pp_name) == 0))
        pp_name = NULL; // for sonar setup
    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "ENTER pname=%s\n", pp_name);
    gv_ms_su_nid = -1;
    gv_ms_su_pid = getpid();
    if (pp_name != NULL) {
        strcpy(ga_ms_su_pname, pp_name);
#ifdef SQ_PHANDLE_VERIFIER
        strcpy(ga_ms_su_pname_seq, ga_ms_su_pname);
#endif
        if (gv_ms_trace_name) {
            sprintf(ga_ms_su_trace_pname, "%s:%d",
                    ga_ms_su_pname, gv_ms_su_pid);
            trace_set_pname(ga_ms_su_trace_pname);
        }
        if (gv_ms_trace_enable)
            trace_where_printf(pp_where, "pname=%s, pid=%d\n",
                               pp_name, getpid());
    } else {
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "clearing su-pname\n");
        memset(ga_ms_su_pname, 0, MS_MON_MAX_PROCESS_NAME);
    }
    memset( la_short_host_name, 0, MPI_MAX_PROCESSOR_NAME );
    gethostname(la_host_name, sizeof(la_host_name));
    char *tmpptr = la_host_name;
    while ( *tmpptr )
    {
        *tmpptr = (char)tolower( *tmpptr );
        tmpptr++;
    }
    // Remove the domain portion of the name if any
    char str1[MPI_MAX_PROCESSOR_NAME];
    memset( str1, 0, MPI_MAX_PROCESSOR_NAME );
    strcpy (str1, la_host_name );

    char *str1_dot = strchr( (char *) str1, '.' );
    if ( str1_dot )
    {
        memcpy( la_short_host_name, str1, str1_dot - str1 );
    }
    else
    {
        strcpy (la_short_host_name, str1 );
    }
    lp_nodes = getenv(gp_ms_env_sq_vnodes);
    lp_nid = getenv(gp_ms_env_sq_vnid);
    if ((lp_nid != NULL) && *lp_nid)
        lv_nid = atoi(lp_nid);
    else
        lv_nid = -1;
    if ((lp_nodes != NULL) && (lv_nid != -1)) {
        sprintf(&lv_file_name,
                "%s/monitor.port.%d.%s",
                getenv(gp_ms_env_mpi_tmpdir),
                lv_nid,
                la_host_name);
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where,
                               "reading %s for SQ_VIRTUAL_NODES=%s and SQ_VIRTUAL_NID=%s\n",
                               &lv_file_name, lp_nodes, lp_nid);
    } else {
        sprintf(&lv_file_name,
                "%s/monitor.port.%s",
                getenv(gp_ms_env_mpi_tmpdir),
                la_short_host_name);
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where,
                               "reading %s (not using virtual nodes)\n",
                               &lv_file_name);
    }
    lp_file = fopen(&lv_file_name, "r");
    if (lp_file != NULL) {
        fgets(ga_ms_su_c_port, MPI_MAX_PORT_NAME, lp_file);
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where,
                               "read %s=%s\n",
                               &lv_file_name, ga_ms_su_c_port);
        fclose(lp_file);
    } else {
        lv_errno = errno;
        if (gv_ms_trace_mon) {
            trace_where_printf(pp_where,
                               "SQ_VIRTUAL_NODES=%s and SQ_VIRTUAL_NID=%s\n",
                               lp_nodes, lp_nid);
            trace_where_printf(pp_where,
                               "EXIT FAILURE could not open file=%s, errno=%d\n",
                               &lv_file_name, lv_errno);
        }
        memset(ga_ms_su_c_port, 0, MPI_MAX_PORT_NAME);
        lv_fserr = XZFIL_ERR_NOTFOUND;
    }
    gv_ms_su_ptype = -1;
    ms_util_fill_phandle_name(&gv_ms_su_phandle,
                              ga_ms_su_pname,
                              gv_ms_su_nid,
                              gv_ms_su_pid
#ifdef SQ_PHANDLE_VERIFIER
                             ,gv_ms_su_verif
#endif
                             );
    return lv_fserr;
}

int msg_mon_init_process_args(const char *pp_where,
                              int        *pp_argc,
                              char     ***pppp_argv) {
    // process args
    if (*pp_argc < 10)
        return ms_err_rtn_msg(pp_where, "argc < 10", XZFIL_ERR_BOUNDSERR);

    // save off startup
    gv_ms_su_pnid = atoi((*pppp_argv)[2]);
    gv_ms_su_nid = atoi((*pppp_argv)[3]);
    // don't use monitor pid - use real pid
    gv_ms_su_pid = getpid();
    strcpy(ga_ms_su_pname, (*pppp_argv)[5]);
    strcpy(ga_ms_su_c_port, (*pppp_argv)[6]);
    gv_ms_su_ptype = atoi((*pppp_argv)[7]);
    gv_ms_su_compid = ms_util_map_ptype_to_compid(gv_ms_su_ptype);
    gv_ms_su_zid = atoi((*pppp_argv)[8]);
#ifdef SQ_PHANDLE_VERIFIER
    gv_ms_su_verif = atoi((*pppp_argv)[9]);
    sprintf(ga_ms_su_pname_seq, "%s:" PFVY, ga_ms_su_pname, gv_ms_su_verif);
#endif
    ms_util_fill_phandle_name(&gv_ms_su_phandle,
                              ga_ms_su_pname,
                              gv_ms_su_nid,
                              gv_ms_su_pid
#ifdef SQ_PHANDLE_VERIFIER
                             ,gv_ms_su_verif
#endif
                             );
    if (gv_ms_trace_name) {
        sprintf(ga_ms_su_trace_pname, "%s:%d/%d",
                ga_ms_su_pname, gv_ms_su_nid, gv_ms_su_pid);
        trace_set_pname(ga_ms_su_trace_pname);
    }
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_enable)
        trace_where_printf(pp_where, "pname=%s, pnid=%d, nid=%d, pid=%d, verif=%d\n",
                           ga_ms_su_pname, gv_ms_su_pnid, gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif);
#else
    if (gv_ms_trace_enable)
        trace_where_printf(pp_where, "pname=%s, pnid=%d, nid=%d, pid=%d\n",
                           ga_ms_su_pname, gv_ms_su_pnid, gv_ms_su_nid, gv_ms_su_pid);
#endif
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "ENTER p-id=%d/%d, pname=%s, port=%s, ptype=%d, zid=%d, verif=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid,
                           ga_ms_su_pname, ga_ms_su_c_port,
                           gv_ms_su_ptype, gv_ms_su_zid, gv_ms_su_verif);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "ENTER p-id=%d/%d, pname=%s, port=%s, ptype=%d, zid=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid,
                           ga_ms_su_pname, ga_ms_su_c_port,
                           gv_ms_su_ptype, gv_ms_su_zid);
#endif

#ifdef SQ_PHANDLE_VERIFIER
    // remove args[1-10]
    for (int lv_arg = 11; lv_arg < *pp_argc; lv_arg++)
        (*pppp_argv)[lv_arg-10] = (*pppp_argv)[lv_arg];
    *pp_argc = *pp_argc - 10;
#else
    // remove args[1-9]
    for (int lv_arg = 10; lv_arg < *pp_argc; lv_arg++)
        (*pppp_argv)[lv_arg-9] = (*pppp_argv)[lv_arg];
    *pp_argc = *pp_argc - 9;
#endif

    if (gv_ms_trace_mon) {
        SB_Buf_Lline lv_line;
        sprintf(&lv_line, "argc=%d. ", *pp_argc);
        for (int lv_arg = 0; lv_arg < *pp_argc; lv_arg++) {
            strcat(&lv_line, "'");
            strcat(&lv_line, (*pppp_argv)[lv_arg]);
            strcat(&lv_line, "'");
            if (lv_arg != (*pp_argc - 1))
                strcat(&lv_line, ",");
        }
        trace_where_printf(pp_where, "%s\n", &lv_line);
    }
    return XZFIL_ERR_OK;
}

SB_Export int msg_mon_mount_device() {
    const char   *WHERE = "msg_mon_mount_device";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_MOUNT_DEVICE);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_MOUNT_DEVICE, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER\n");
    if (!gv_ms_mon_calls_ok) // msg_mon_mount_device
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Mount;
    lp_msg->u.request.u.mount.nid = gv_ms_su_nid;
    lp_msg->u.request.u.mount.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.mount.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.mount.process_name,
                         sizeof(lp_msg->u.request.u.mount.process_name));
#endif
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send mount req to mon, p-id=%d/%d/" PFVY "\n",
                           gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send mount req to mon, p-id=%d/%d\n",
                           gv_ms_su_nid, gv_ms_su_pid);
#endif
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "mount",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "mount req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Mount)) {
        lv_mpierr = lp_msg->u.reply.u.mount.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK mount req\n");
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE mount, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);

}

SB_Export int msg_mon_mount_device2(MS_MON_DEVICE_STATE *pp_primary,
                                    MS_MON_DEVICE_STATE *pp_mirror) {
    const char   *WHERE = "msg_mon_mount_device2";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_MOUNT_DEVICE);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_MOUNT_DEVICE, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER primary=%p, mirror=%p\n",
                          pfp(pp_primary), pfp(pp_mirror));
    if (!gv_ms_mon_calls_ok) // msg_mon_mount_device2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Mount;
    lp_msg->u.request.u.mount.nid = gv_ms_su_nid;
    lp_msg->u.request.u.mount.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.mount.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.mount.process_name,
                         sizeof(lp_msg->u.request.u.mount.process_name));
#endif
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send mount req to mon, p-id=%d/%d/" PFVY "\n",
                           gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send mount req to mon, p-id=%d/%d\n",
                           gv_ms_su_nid, gv_ms_su_pid);
#endif
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "mount",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "mount req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Mount)) {
        lv_mpierr = lp_msg->u.reply.u.mount.return_code;
        *pp_primary = static_cast<MS_MON_DEVICE_STATE>(lp_msg->u.reply.u.mount.primary_state);
        *pp_mirror = static_cast<MS_MON_DEVICE_STATE>(lp_msg->u.reply.u.mount.mirror_state);
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK mount req, primary=%d, mirror=%d\n",
                                  lp_msg->u.reply.u.mount.primary_state,
                                  lp_msg->u.reply.u.mount.mirror_state);
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE mount, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);

}

MS_Md_Type *msg_mon_loc_get_md(Mon_Msg_Type *pp_msg, int pv_size) {
    const char   *WHERE = "msg_mon_loc_get_md";
    Mon_Msg_Type *lp_msg;

    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MON_MSG,
                       pp_msg->type,
                       pp_msg->u.request.type);
    // make a copy
    lp_msg =
      static_cast<Mon_Msg_Type *>(MS_BUF_MGR_ALLOC(pv_size));
    memcpy(lp_msg, pp_msg, pv_size);
    if ((pp_msg->type == MsgType_Close) && gv_ms_trace_locio) {
        trace_where_printf(WHERE,
                           "close %s, ab=%d, cclose %s, cab'%d\n",
                           pp_msg->u.request.u.close.process_name,
                           pp_msg->u.request.u.close.aborted,
                           lp_msg->u.request.u.close.process_name,
                           lp_msg->u.request.u.close.aborted);
        lp_msg->u.request.u.close.aborted = 0;
    }
    // fill out md for delivery
    MS_Md_Type *lp_md;
    SB_Trans::Trans_Stream *lp_stream = SB_Trans::Trans_Stream::get_mon_stream();
    SB_Trans::Msg_Mgr::get_md(&lp_md, // mon_msg_loc_get_md
                              lp_stream,
                              NULL,
                              false,  // recv
                              NULL,   // fserr
                              WHERE,
                              MD_STATE_RCVD_MON_MSG);
    SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
    lp_md->iv_tag = -1;
    lp_md->out.iv_mon_msg = true;
    lp_md->out.iv_ldone = false;
    lp_md->out.iv_nid = gv_ms_su_nid;
    lp_md->out.iv_pid = gv_ms_su_pid;
    lp_md->out.iv_recv_req_id = 0;
    lp_md->out.iv_pri = -1;
    lp_md->out.iv_recv_mpi_source_rank = 0;
    lp_md->out.iv_recv_mpi_tag = 0;
    lp_md->out.ip_recv_ctrl = NULL;
    lp_md->out.iv_recv_ctrl_size = 0;
    lp_md->out.ip_recv_data = reinterpret_cast<char *>(lp_msg);
    lp_md->out.iv_recv_data_size = pv_size;
    lp_md->out.iv_recv_ctrl_max = 0;
    lp_md->out.iv_recv_data_max = 0;

    return lp_md;
}

//
// Purpose: Check if mon msg ok
//
bool msg_mon_msg_ok(const char   *pp_where,
                    const char   *pp_where_detail,
                    int          *pp_mpierr,
                    Mon_Msg_Type *pp_msg,
                    int           pv_msg_type,
                    int           pv_reply_type) {
    const char *lp_exp_msg_type;
    const char *lp_exp_reply_type;
    const char *lp_msg_type;
    const char *lp_reply_type;

    if (gv_ms_trace_mon) {
        lp_msg_type = msg_util_get_msg_type(pp_msg->type);
        lp_reply_type = msg_util_get_reply_type(pp_msg->u.reply.type);
        trace_where_printf(pp_where, "%s mpierr=%d, reply msg type=%d(%s), reply type=%d(%s)\n",
                           pp_where_detail,
                           *pp_mpierr,
                           pp_msg->type,
                           lp_msg_type,
                           pp_msg->u.reply.type,
                           lp_reply_type);
    }

    if (*pp_mpierr != MPI_SUCCESS)
        return false;

    if (pp_msg->type != pv_msg_type) {
        if (gv_ms_trace_mon) {
            lp_msg_type = msg_util_get_msg_type(pp_msg->type);
            lp_exp_msg_type =
              msg_util_get_msg_type(static_cast<MSGTYPE>(pv_msg_type));
            trace_where_printf(pp_where, "%s invalid message type, msg type=%d(%s), exp type=%d(%s)\n",
                               pp_where_detail,
                               pp_msg->type,
                               lp_msg_type,
                               pv_msg_type,
                               lp_exp_msg_type);
        }
        *pp_mpierr = MPI_ERR_INTERN;
        return false;
    }

    if (pp_msg->u.reply.type != pv_reply_type) {
        if (pp_msg->u.reply.type == ReplyType_Generic) {
            if (pp_msg->u.reply.u.generic.return_code != MPI_SUCCESS) {
                *pp_mpierr = pp_msg->u.reply.u.generic.return_code;
                if (gv_ms_trace_mon)
                    trace_where_printf(pp_where, "%s generic reply returned mpierr=%d\n",
                                       pp_where_detail,
                                       *pp_mpierr);
                return false;
            }
            // generic returned MPI_SUCCESS - this should be an error
        }

        if (gv_ms_trace_mon) {
            lp_reply_type = msg_util_get_reply_type(pp_msg->u.reply.type);
            lp_exp_reply_type =
              msg_util_get_reply_type(static_cast<REPLYTYPE>(pv_reply_type));
            trace_where_printf(pp_where, "%s invalid reply type, reply type=%d(%s), exp type=%d(%s)\n",
                               pp_where_detail,
                               pp_msg->u.reply.type,
                               lp_reply_type,
                               pv_reply_type,
                               lp_exp_reply_type);
        }
        *pp_mpierr = MPI_ERR_INTERN;
        return false;
    }
    return true;
}

//
// Purpose: node down
//
SB_Export int msg_mon_node_down(int pv_nid) {
    const char   *WHERE = "msg_mon_node_down";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_NODE_DOWN);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_NODE_DOWN, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER nid=%d\n", pv_nid);
    if (!gv_ms_mon_calls_ok) // msg_mon_node_down
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    // send the get to the monitor
    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = true;
    lp_msg->u.request.type = ReqType_NodeDown;
    lp_msg->u.request.u.down.nid = pv_nid;
    lp_msg->u.request.u.down.node_name[0] = '\0';
    lp_msg->u.request.u.down.reason[0] = '\0';
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send node-down req to mon, p-id=%d/%d, nid=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid,
                           pv_nid);
    lv_mpierr = lv_msg.get_error();
    if (lv_mpierr == MPI_SUCCESS) {
        if (gp_local_mon_io->send(lp_msg)) {
            lv_mpierr = ms_err_errno_to_mpierr(WHERE);
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_SEND,
                               errno,
                               lv_mpierr);
        } else
            lv_mpierr = MPI_SUCCESS;
    }
    if (lv_mpierr != MPI_SUCCESS)
        return ms_err_mpi_rtn_msg_fatal(WHERE, "node-down: local-io-send failed",
                                        lv_mpierr);
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: node down
//
SB_Export int msg_mon_node_down2(int pv_nid, const char *pp_reason) {
    const char   *WHERE = "msg_mon_node_down2";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_NODE_DOWN2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_NODE_DOWN2, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER nid=%d, reason=%s\n", pv_nid, pp_reason);
    if (!gv_ms_mon_calls_ok) // msg_mon_node_down
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    // send the get to the monitor
    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = true;
    lp_msg->u.request.type = ReqType_NodeDown;
    lp_msg->u.request.u.down.nid = pv_nid;
    lp_msg->u.request.u.down.node_name[0] = '\0';
    ms_util_string_copy(lp_msg->u.request.u.down.reason,
                        sizeof(lp_msg->u.request.u.down.reason),
                        pp_reason);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send node-down req to mon, p-id=%d/%d, nid=%d, reason=%s\n",
                           gv_ms_su_nid, gv_ms_su_pid,
                           pv_nid,
                           pp_reason);
    lv_mpierr = lv_msg.get_error();
    if (lv_mpierr == MPI_SUCCESS) {
        if (gp_local_mon_io->send(lp_msg)) {
            lv_mpierr = ms_err_errno_to_mpierr(WHERE);
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_SEND,
                               errno,
                               lv_mpierr);
        } else
            lv_mpierr = MPI_SUCCESS;
    }
    if (lv_mpierr != MPI_SUCCESS)
        return ms_err_mpi_rtn_msg_fatal(WHERE, "node-down: local-io-send failed",
                                        lv_mpierr);
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: node up
//
SB_Export int msg_mon_node_up(int pv_nid) {
    const char   *WHERE = "msg_mon_node_up";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_NODE_UP);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_NODE_UP, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER nid=%d\n", pv_nid);
    if (!gv_ms_mon_calls_ok) // msg_mon_node_up
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    // send the get to the monitor
    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = true;
    lp_msg->u.request.type = ReqType_NodeUp;
    lp_msg->u.request.u.up.nid = pv_nid;
    lp_msg->u.request.u.up.node_name[0] = '\0';
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send node-up req to mon, p-id=%d/%d, nid=%d\n",
                           gv_ms_su_nid, gv_ms_su_pid,
                           pv_nid);
    lv_mpierr = lv_msg.get_error();
    if (lv_mpierr == MPI_SUCCESS) {
        if (gp_local_mon_io->send(lp_msg)) {
            lv_mpierr = ms_err_errno_to_mpierr(WHERE);
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_SEND,
                               errno,
                               lv_mpierr);
        } else
            lv_mpierr = MPI_SUCCESS;
    }
    if (lv_mpierr != MPI_SUCCESS)
        return ms_err_mpi_rtn_msg_fatal(WHERE, "node-up: local-io-send failed",
                                        lv_mpierr);
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: handle opening process
//
SB_Export int msg_mon_open_process(char            *pp_name,
                                   SB_Phandle_Type *pp_phandle,
                                   int             *pp_oid) {
    SB_API_CTR (lv_zctr, MSG_MON_OPEN_PROCESS);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_OPEN_PROCESS, 0);
    return msg_mon_open_process_com(pp_name,
                                    pp_phandle,
                                    pp_oid,
                                    false,   // reopen
                                    true,    // death notification
                                    false,   // self
                                    false,   // backup
                                    false,   // ic
                                    false);  // fs
}

//
// Purpose: handle opening process
//
SB_Export int msg_mon_open_process_fs(char            *pp_name,
                                      SB_Phandle_Type *pp_phandle,
                                      int             *pp_oid) {
    SB_API_CTR (lv_zctr, MSG_MON_OPEN_PROCESS_FS);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_OPEN_PROCESS, 0);
    return msg_mon_open_process_com(pp_name,
                                    pp_phandle,
                                    pp_oid,
                                    false,   // reopen
                                    true,    // death notification
                                    false,   // self
                                    false,   // backup
                                    false,   // ic
                                    true);   // fs
}

//
// Purpose: handle opening process
//
SB_Export int msg_mon_open_process_ic(char            *pp_name,
                                      SB_Phandle_Type *pp_phandle,
                                      int             *pp_oid) {
    SB_API_CTR (lv_zctr, MSG_MON_OPEN_PROCESS_IC);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_OPEN_PROCESS_IC, 0);
    return msg_mon_open_process_com(pp_name,
                                    pp_phandle,
                                    pp_oid,
                                    false,   // reopen
                                    true,    // death notification
                                    false,   // self
                                    false,   // backup
                                    true,    // ic
                                    false);  // fs
}

SB_Export int msg_mon_open_process_backup(char            *pp_name,
                                          SB_Phandle_Type *pp_phandle,
                                          int             *pp_oid) {
    SB_API_CTR (lv_zctr, MSG_MON_OPEN_PROCESS_BACKUP);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_OPEN_PROCESS_BACKUP, 0);
    return msg_mon_open_process_com(pp_name,
                                    pp_phandle,
                                    pp_oid,
                                    false,  // reopen
                                    false,  // death notification
                                    false,  // self
                                    true,   // backup
                                    false,  // ic
                                    false); // fs
}

SB_Export int msg_mon_open_process_self(SB_Phandle_Type *pp_phandle,
                                        int             *pp_oid) {
    SB_API_CTR (lv_zctr, MSG_MON_OPEN_PROCESS_SELF);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_OPEN_PROCESS_SELF, 0);
    return msg_mon_open_process_com(ga_ms_su_pname,
                                    pp_phandle,
                                    pp_oid,
                                    false,  // reopen
                                    true,   // death notification
                                    true,   // self!
                                    false,  // backup
                                    false,  // ic
                                    false); // fs
}

SB_Export int msg_mon_open_process_self_ic(SB_Phandle_Type *pp_phandle,
                                           int             *pp_oid) {
    SB_API_CTR (lv_zctr, MSG_MON_OPEN_PROCESS_SELF_IC);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_OPEN_PROCESS_SELF_IC, 0);
    return msg_mon_open_process_com(ga_ms_su_pname,
                                    pp_phandle,
                                    pp_oid,
                                    false,  // reopen
                                    true,   // death notification
                                    true,   // self!
                                    false,  // backup
                                    true,   // ic
                                    false); // fs
}

//
// Purpose: complete inline-open/close (callback-target)
//
void msg_mon_oc_cbt(MS_Md_Type *pp_md, void *pp_stream) {
    const char   *WHERE = "msg_mon_oc_cbt";
    Mon_Msg_Type *lp_msg = NULL;
    bool          lv_aborted;
    bool          lv_close_dup;
    bool          lv_close_shutdown;
    int           lv_size;
    int           lv_status;

    lv_aborted = false;
    lv_close_dup = false;
    lv_close_shutdown = false;

    // TODO: if can't get buffer
    switch (pp_md->out.iv_msg_type) {
    case MS_PMH_TYPE_CLOSE:
        lv_aborted = (pp_md->out.iv_recv_mpi_source_rank == -2);
        if (lv_aborted && gv_ms_recv_q_proc_death)
            ms_recv_q_proc_death(pp_md->out.iv_nid,
                                 pp_md->out.iv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                 pp_md->out.iv_verif,
#endif
                                 true,
                                 pp_stream);
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "close rcvd, aborted=%d\n", lv_aborted);
#ifdef SQ_PHANDLE_VERIFIER
        if (!msg_mon_conn_state_get(pp_md->out.iv_nid, pp_md->out.iv_pid, pp_md->out.iv_verif)) {
#else
        if (!msg_mon_conn_state_get(pp_md->out.iv_nid, pp_md->out.iv_pid)) {
#endif
            lv_close_dup = true;
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "p-id=%d/%d not in conn-state-map, set dup\n",
                                   pp_md->out.iv_nid, pp_md->out.iv_pid);
            if (!lv_aborted)
                break;
        }

        // If the process is being shutdown the close message
        // can be discarded

        if (gv_ms_shutdown_called) {
            lv_close_shutdown = true;
            break;
        }

        lv_size = static_cast<int>(offsetof(Mon_Msg_Type, u.request.u.close) +
                                   sizeof(lp_msg->u.request.u.close));
        lp_msg = static_cast<Mon_Msg_Type *>(MS_BUF_MGR_ALLOC(lv_size));
        pp_md->out.ip_recv_data = reinterpret_cast<char *>(lp_msg);
        pp_md->out.iv_recv_data_size = lv_size;
        lp_msg->type = MsgType_Close;
        lp_msg->u.request.type = ReqType_Notice;
        lp_msg->u.request.u.close.nid = pp_md->out.iv_nid;
        lp_msg->u.request.u.close.pid = pp_md->out.iv_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lp_msg->u.request.u.close.verifier = pp_md->out.iv_verif;
#endif
        ms_util_string_copy(lp_msg->u.request.u.close.process_name,
                            sizeof(lp_msg->u.request.u.close.process_name),
                            pp_md->out.ip_recv_ctrl);
        if (lv_aborted) {
            lp_msg->u.request.u.close.aborted = true;
            lp_msg->u.request.u.close.mon = true;
        } else {
            lp_msg->u.request.u.close.aborted = false;
            lp_msg->u.request.u.close.mon = false;
        }
        msg_mon_conn_state_change(lp_msg->u.request.u.close.nid,
                                  lp_msg->u.request.u.close.pid,
#ifdef SQ_PHANDLE_VERIFIER
                                  lp_msg->u.request.u.close.verifier,
#endif
                                  lp_msg->type);
        break;

    case MS_PMH_TYPE_OPEN:
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "open rcvd\n");
        lv_size = static_cast<int>(offsetof(Mon_Msg_Type, u.request.u.open) +
                                   sizeof(lp_msg->u.request.u.open));
        lp_msg = static_cast<Mon_Msg_Type *>(MS_BUF_MGR_ALLOC(lv_size));
        pp_md->out.ip_recv_data = reinterpret_cast<char *>(lp_msg);
        pp_md->out.iv_recv_data_size = lv_size;
        lp_msg->type = MsgType_Open;
        lp_msg->u.request.type = ReqType_Notice;
        lp_msg->u.request.u.open.nid = pp_md->out.iv_nid;
        lp_msg->u.request.u.open.pid = pp_md->out.iv_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lp_msg->u.request.u.open.verifier = pp_md->out.iv_verif;
#endif
        ms_util_string_copy(lp_msg->u.request.u.open.target_process_name,
                            sizeof(lp_msg->u.request.u.open.target_process_name),
                            pp_md->out.ip_recv_ctrl);
        lp_msg->u.request.u.open.death_notification = false;
        msg_mon_conn_state_change(lp_msg->u.request.u.open.nid,
                                  lp_msg->u.request.u.open.pid,
#ifdef SQ_PHANDLE_VERIFIER
                                  lp_msg->u.request.u.open.verifier,
#endif
                                  lp_msg->type);
        break;

    default:
        SB_util_abort("invalid pp_md->out.iv_msg_type"); // sw fault
        break;
    }

    //
    // At this point, for a close, there are five possibilities:
    //   close-shutdown
    //     put-md - nothing to do
    //   close-duplicate && !aborted
    //     put-md - nothing to do
    //   close-duplicate && aborted
    //     put-md - nothing to do
    //   !close-duplicate && !aborted
    //     deliver to application
    //   !close-duplicate && aborted
    //     deliver to application
    //

    if (lv_close_shutdown) {
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "connection-closed, discard close during shutdown\n");
        ms_free_recv_bufs(pp_md);
        SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "shutdown CLOSE");
    } else if (lv_close_dup) {
        if (lv_aborted) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "connection-closed (aborted)\n");
            ms_free_recv_bufs(pp_md);
            SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "aborted CLOSE");
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "connection-closed, discard duplicate close\n");
            ms_free_recv_bufs(pp_md);
            SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "duplicate CLOSE");
        }
    } else {
        if (lv_aborted) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "connection-open (aborted)\n");
        }

        if (gv_ms_trace_mon && NULL != lp_msg) {
            const char *lp_msg_type = msg_util_get_msg_type(lp_msg->type);
            trace_where_printf(WHERE, "manufacturing/queueing %s (%d) mon message\n",
                               lp_msg_type, lp_msg->type);
            msg_mon_trace_msg(WHERE, lp_msg);
        }
        if (gv_ms_enable_messages && NULL != lp_msg) {
            if (lp_msg->type == MsgType_Close) {

                do {
                    lv_status = SB_Trans::Trans_Stream::map_nidpid_trylock();
                    if (lv_status == EBUSY)
                        usleep(100);
                } while (lv_status);
                SB_util_assert_ieq(lv_status, 0 );

                SB_Trans::Trans_Stream *lp_stream =
                  SB_Trans::Trans_Stream::map_nidpid_to_stream(lp_msg->u.request.u.close.nid,
                                                               lp_msg->u.request.u.close.pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                               lp_msg->u.request.u.close.verifier,
#endif
                                                               false);
                if (lp_stream == NULL)
                    lp_stream = SB_Trans::Trans_Stream::get_mon_stream();

                if (lp_stream != NULL && lp_stream->ref_get() > 0) {
                    lp_stream->post_mon_mutex_lock();

                    if (lp_stream->post_mon_messages_get()) {
                        if (gv_ms_trace_mon)
                            trace_where_printf(WHERE,"close inside processing stream %s - death message not yet posted, so post close\n", lp_stream->get_name());

                        if ((gv_ms_lim_cb != NULL) &&
                            gv_ms_lim_cb(0, // mon-msg
                                         NULL,
                                         0,
                                         pp_md->out.ip_recv_data,
                                         pp_md->out.iv_recv_data_size)) {
                            gv_ms_lim_q.add(&pp_md->iv_link);
                            gv_ms_event_mgr.set_event_all(INTR);
                        } else {
                            gv_ms_recv_q.add(&pp_md->iv_link);
                            gv_ms_event_mgr.set_event_all(LREQ);
                        }
                    } else {
                        if (gv_ms_trace_mon)
                            trace_where_printf(WHERE, "discarding mon death md\n");
                        gv_ms_buf_mgr.uf.ifree(pp_md->out.ip_recv_data);
                        pp_md->out.ip_recv_data = NULL;
                        SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "recv-process-death-md-free");
                    }

                    lp_stream->post_mon_mutex_unlock();
                } else {
                    if (gv_ms_trace_mon)
                        trace_where_printf(WHERE, "discarding mon death md\n");
                    gv_ms_buf_mgr.uf.ifree(pp_md->out.ip_recv_data);
                    pp_md->out.ip_recv_data = NULL;
                    SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "recv-process-death-md-free");
                }


                SB_Trans::Trans_Stream::map_nidpid_unlock();

            } else {
                if ((gv_ms_lim_cb != NULL) &&
                    gv_ms_lim_cb(0, // mon-msg
                                 NULL,
                                 0,
                                 pp_md->out.ip_recv_data,
                                 pp_md->out.iv_recv_data_size)) {
                    gv_ms_lim_q.add(&pp_md->iv_link);
                    gv_ms_event_mgr.set_event_all(INTR);
                } else {
                    gv_ms_recv_q.add(&pp_md->iv_link);
                    gv_ms_event_mgr.set_event_all(LREQ);
                }
            }
        } else
            msg_mon_recv_msg_cbt_discard(WHERE, pp_md, false);
    }
}

//
// Purpose: handle opening process
//
int msg_mon_open_process_com(char            *pp_name,
                             SB_Phandle_Type *pp_phandle,
                             int             *pp_oid,
                             bool             pv_reopen,
                             bool             pv_death_notif,
                             bool             pv_self,
                             bool             pv_backup,
                             bool             pv_ic,
                             bool             pv_fs_open) {
    char        la_name[MS_MON_MAX_PROCESS_NAME+1];
    Ms_Od_Type *lp_od;
    int         lv_done;
    int         lv_fserr;

    SB_util_get_case_insensitive_name(pp_name, la_name);

    if (pv_self) {
        lv_fserr = msg_mon_open_self(la_name, pp_phandle, pp_oid, pv_ic);
        return lv_fserr;
    }
    lv_fserr = msg_mon_open_process_com_ph1(la_name,
                                            pp_phandle,
                                            pp_oid,
                                            pv_reopen,
                                            pv_death_notif,
                                            pv_backup,
                                            pv_fs_open,
                                            &lp_od,
                                            &lv_done);
    if (lv_done)
        return lv_fserr;
    lv_fserr = msg_mon_open_process_com_ph2(la_name,
                                            pp_phandle,
                                            pp_oid,
                                            pv_reopen,
                                            pv_death_notif,
                                            pv_ic,
                                            lp_od);
    return lv_fserr;
}

int msg_mon_open_process_com_ph1(char             *pp_name,
                                 SB_Phandle_Type  *pp_phandle,
                                 int              *pp_oid,
                                 bool              pv_reopen,
                                 bool              pv_death_notif,
                                 bool              pv_backup,
                                 bool              pv_fs_open,
                                 Ms_Od_Type      **ppp_od,
                                 int              *pp_done) {
    const char   *WHERE = "msg_mon_open_process-ph1";
    Ms_Od_Type   *lp_od;
    int           lv_acc_count;
    int           lv_con_count;
    int           lv_fserr;
    SB_Buf_Lline  lv_line;
#ifdef USE_SB_SP_LOG
    int           lv_status;
#endif
    int           lv_total_count;

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=%s, phandle=%p, oid=%p, reopen=%d, death_notif=%d, backup=%d, fs=%d\n",
                           pp_name,
                           pfp(pp_phandle),
                           pfp(pp_oid),
                           pv_reopen,
                           pv_death_notif,
                           pv_backup,
                           pv_fs_open);
    *pp_done = true;
    if (!gv_ms_calls_ok) // msg_mon_open_process
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (!pv_reopen) {
        if (strlen(pp_name) > SB_PHANDLE_NAME_SIZE) {
            sprintf(&lv_line, "process name limited to %d characters",
                    SB_PHANDLE_NAME_SIZE);
            return ms_err_rtn_msg(WHERE, &lv_line, XZFIL_ERR_BOUNDSERR);
        }
        lp_od = ms_od_map_name_to_od(pp_name);
        if (lp_od != NULL) {
            lv_fserr = XZFIL_ERR_OK;
            int lv_ref = ms_od_ref_add(lp_od);
            if (lp_od->ip_stream == NULL) { // no stream
                SB_Trans::Trans_Stream::add_stream_con_count(1); // need to bump
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "EXIT OK need monitor open (no stream), references=%d\n",
                                       lv_ref);
                if (pp_oid != NULL)
                    *pp_oid = 0; // initialize
                *ppp_od = lp_od;
                *pp_done = false;
                return XZFIL_ERR_OK;
            }
            if (lp_od->iv_need_open && (lv_ref == 1)) {
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "send inline open, references=%d\n",
                                       lv_ref);
                // send open to remote
                SB_Trans::Stream_Base *lp_stream = lp_od->ip_stream;
                if (gv_ms_conn_idle_timeout > 0)
                    gv_ms_idle_timer_map.remove(lp_stream->get_idle_timer_link());
                MS_Md_Type *lp_open_md;
                SB_Trans::Msg_Mgr::get_md(&lp_open_md, // msg_mon_open_process
                                          lp_stream,
                                          NULL,
                                          true,        // send
                                          NULL,        // fserr
                                          "open (msg_mon_open_process)",
                                          MD_STATE_SEND_INLINE_OPEN);
                SB_util_assert_pne(lp_open_md, NULL); // TODO: can't get md
                lp_open_md->iv_inline = true;
                lv_fserr = lp_stream->exec_open(lp_open_md,
                                                lp_open_md->iv_link.iv_id.i); // reqid
                if (lv_fserr == XZFIL_ERR_OK) {
                    lp_stream->wait_req_done(lp_open_md);
                    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_REQ_DONE,
                                       lp_open_md->iv_link.iv_id.i);
                    if (gv_ms_trace)
                        trace_where_printf(WHERE, "open reply-done=1\n");
                    lv_fserr = lp_open_md->out.iv_fserr;
                    if (lv_fserr != XZFIL_ERR_OK) {
                        SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_EXIT, lv_fserr);
                    }
                } else {
                    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_EXIT, lv_fserr);
                }
                MS_BUF_MGR_FREE(lp_open_md->out.ip_recv_ctrl);
                SB_Trans::Msg_Mgr::put_md(lp_open_md->iv_link.iv_id.i,
                                          "finish-send OPEN");
            }
            if (lv_fserr == XZFIL_ERR_OK) {
                ms_util_fill_phandle_name(pp_phandle,
                                          pp_name,
                                          lp_od->iv_nid,
                                          lp_od->iv_pid
#ifdef SQ_PHANDLE_VERIFIER
                                         ,lp_od->iv_verif
#endif
                                         );
                ms_util_fill_phandle_oid(pp_phandle, lp_od->iv_oid);
                if (gv_ms_trace_mon) {
                    char la_phandle[MSG_UTIL_PHANDLE_LEN];
                    msg_util_format_phandle(la_phandle, pp_phandle);
                    trace_where_printf(WHERE, "EXIT OK no monitor open, reuse-open, references=%d, phandle=%s\n",
                                       lv_ref, la_phandle);
                }
                if (pp_oid != NULL)
                    *pp_oid = lp_od->iv_oid;
                return XZFIL_ERR_OK;
            } else {
                SB_Trans::Trans_Stream *lp_stream = lp_od->ip_stream;
                if (lp_stream != NULL) {
                    if (gv_ms_trace_mon)
                        trace_where_printf(WHERE, "closing stream for %s\n",
                                           lp_stream->get_name());
                    // close will dec
                    SB_Trans::Trans_Stream::add_stream_con_count(1);
                    SB_Trans::Trans_Stream::close_stream(WHERE,
                                                         lp_stream,
                                                         true,
                                                         true,
                                                         false);
                    lp_od->ip_stream = NULL;
                }
            }
        } else {
            SB_Trans::Trans_Stream::add_stream_con_count(1);
            lv_total_count = SB_Trans::Trans_Stream::get_stream_total_count();
            lv_con_count = SB_Trans::Trans_Stream::get_stream_con_count();
            lv_acc_count = SB_Trans::Trans_Stream::get_stream_acc_count();
            lv_fserr = XZFIL_ERR_OK;
            if ((gv_ms_streams_max > 0) &&
                (lv_total_count > gv_ms_streams_max))
                lv_fserr = XZFIL_ERR_NOBUFSPACE;
            else {
            }
            if (lv_fserr != XZFIL_ERR_OK) {
                SB_Trans::Trans_Stream::add_stream_con_count(-1);
                sprintf(&lv_line, "where=%s, total-streams=%d, con-streams(est)=%d, acc-streams(est)=%d, max-streams=%d, fserr=%d\n",
                        WHERE, lv_total_count, lv_con_count, lv_acc_count, gv_ms_streams_max, lv_fserr);
#ifdef USE_SB_SP_LOG
                lv_status = SB_log_write_str(SQEVL_SEABED, // USE_SB_SP_LOG
                                             SB_EVENT_ID,
                                             SQ_LOG_SEAQUEST,
                                             SQ_LOG_INFO,
                                             &lv_line);
                CHK_STATUSIGNORE(lv_status);
#endif
                if (gv_ms_trace_mon || gv_ms_trace_errors) {
                    trace_where_printf(WHERE, &lv_line);
                    trace_where_printf(WHERE, "EXIT FAILURE all streams used\n");
                }
                return lv_fserr;
            }
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "con-streams=%d, acc-streams=%d, max-streams=%d\n",
                                   lv_con_count, lv_acc_count,
                                   gv_ms_streams_max);
            lp_od = ms_od_alloc();
            if (lp_od == NULL) {
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "EXIT FAILURE could not alloc od\n");
                SB_Trans::Trans_Stream::add_stream_con_count(-1);
                return XZFIL_ERR_NOBUFSPACE;
            }
            lp_od->iv_death_notif = pv_death_notif;
            lp_od->iv_fs_open = pv_fs_open;
        }
    } else {
        lp_od = ms_od_map_phandle_to_od(pp_phandle);
        pp_oid = NULL; // reopen - don't set oid
    }

    if (pp_oid != NULL)
        *pp_oid = 0; // initialize
    *ppp_od = lp_od;
    *pp_done = false;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");
    return XZFIL_ERR_OK;
}

int msg_mon_open_process_com_ph2(char            *pp_name,
                                 SB_Phandle_Type *pp_phandle,
                                 int             *pp_oid,
                                 bool             pv_reopen,
                                 bool             pv_death_notif,
                                 bool             pv_ic,
                                 Ms_Od_Type      *pp_od) {
    const char   *WHERE = "msg_mon_open_process-ph2";
    Mon_Msg_Type *lp_msg;
    int           lv_fserr;
    int           lv_mpierr;

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Open;
    lp_msg->u.request.u.open.nid = gv_ms_su_nid;
    lp_msg->u.request.u.open.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.open.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.open.process_name,
                         sizeof(lp_msg->u.request.u.open.process_name));
    lp_msg->u.request.u.open.target_nid = -1;
    lp_msg->u.request.u.open.target_pid = -1;
    ms_util_name_seq(pp_name,
                     lp_msg->u.request.u.open.target_process_name,
                     sizeof(lp_msg->u.request.u.open.target_process_name),
                     &lp_msg->u.request.u.open.target_verifier);
#else
    ms_util_string_copy(lp_msg->u.request.u.open.target_process_name,
                        sizeof(lp_msg->u.request.u.open.target_process_name),
                        pp_name);
#endif
    lp_msg->u.request.u.open.death_notification = pv_death_notif;
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send open req to mon, p-id=%d/%d/" PFVY ", t-name=%s:" PFVY "\n",
                           gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif,
                           lp_msg->u.request.u.open.target_process_name,
                           lp_msg->u.request.u.open.target_verifier);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send open req to mon, p-id=%d/%d, pname=%s\n",
                           gv_ms_su_nid, gv_ms_su_pid, pp_name);
#endif
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "open",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "open req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Open)) {
        lv_mpierr = lp_msg->u.reply.u.open.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            int lv_nid = lp_msg->u.reply.u.open.nid;
            int lv_pid = lp_msg->u.reply.u.open.pid;
#ifdef SQ_PHANDLE_VERIFIER
            SB_Verif_Type lv_verif = lp_msg->u.reply.u.open.verifier;
#endif
            int lv_ptype = lp_msg->u.reply.u.open.type;
            if (gv_ms_trans_sock) {
                lv_fserr = msg_mon_open_process_com_ph2_sock(WHERE,
                                                             pp_name,
                                                             pp_phandle,
                                                             pp_oid,
                                                             pv_reopen,
                                                             pv_ic,
                                                             pp_od,
                                                             lp_msg,
                                                             lv_nid,
                                                             lv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                             lv_verif,
#endif
                                                             lv_ptype);
            } else
                lv_fserr = XZFIL_ERR_OK;
            if (lv_fserr != XZFIL_ERR_OK) {
                // ms_od_free already called by msg_mon_open_process_com_ph2...
                return ms_err_rtn_msg(WHERE, "EXIT",
                                      static_cast<short>(lv_fserr));
            }
        } else {
            ms_od_free(pp_od, !pv_reopen, false, pv_reopen);
            if (!pv_reopen)
                SB_Trans::Trans_Stream::add_stream_con_count(-1);
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE open req, ret=%d\n",
                                   lv_mpierr);
        }
    } else {
        ms_od_free(pp_od, !pv_reopen, false, pv_reopen);
        if (!pv_reopen)
            SB_Trans::Trans_Stream::add_stream_con_count(-1);
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

int msg_mon_open_process_com_ph2_sock(const char      *pp_where,
                                      char            *pp_name,
                                      SB_Phandle_Type *pp_phandle,
                                      int             *pp_oid,
                                      bool             pv_reopen,
                                      bool             pv_ic,
                                      Ms_Od_Type      *pp_od,
                                      Mon_Msg_Type    *pp_msg,
                                      int              pv_nid,
                                      int              pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                      SB_Verif_Type    pv_verif,
#endif
                                      int              pv_ptype) {
    char          la_prog[SB_MAX_PROG];

    if (gv_ms_trace_mon) {
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(pp_where, "open req OK, p-id=%d/%d/" PFVY ", port=%s\n",
#else
        trace_where_printf(pp_where, "open req OK, p-id=%d/%d, port=%s\n",
#endif
                           pv_nid,
                           pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                           pv_verif,
#endif
                           pp_msg->u.reply.u.open.port);
        trace_where_printf(pp_where, "attempting to connect to port=%s\n",
                           pp_msg->u.reply.u.open.port);
    }
    SB_Trans::Sock_Client *lp_sock = new SB_Trans::Sock_Client();
    lp_sock->set_sdp(gv_ms_ic_ibv);
    lp_sock->connect(pp_msg->u.reply.u.open.port);
    if (gv_ms_trace_mon) {
        trace_where_printf(pp_where, "connected to process, pname=%s\n",
                           pp_name);
        trace_where_printf(pp_where,
                           "creating stream for sock=%d\n",
                           lp_sock->get_sock());
    }
    int lv_fserr = msg_mon_accept_sock_negotiate_id_cli(lp_sock,
                                                        pv_nid,
                                                        pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                        pv_verif,
#endif
                                                        pv_ic,
                                                        la_prog);
    if (lv_fserr != XZFIL_ERR_OK) {
        // backout
        pp_od->ip_stream = NULL;
        ms_od_free(pp_od, !pv_reopen, false, false);

        // trace
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "EXIT FAILURE negotiate id, ret=%d\n",
                               lv_fserr);
        return lv_fserr;
    }
    SB_Buf_Line la_name;
#ifdef SQ_PHANDLE_VERIFIER
    sprintf(la_name, "connect p-id=%d/%d/" PFVY " (%s-%s)",
#else
    sprintf(la_name, "connect p-id=%d/%d (%s-%s)",
#endif
#ifdef SQ_PHANDLE_VERIFIER
            pv_nid, pv_pid, pv_verif, pp_name, la_prog);
#else
            pv_nid, pv_pid, pp_name, la_prog);
#endif
    strcpy(pp_od->ia_prog, la_prog);
    if (pv_ic)
        strcat(la_name, "-IC");
    SB_Trans::Trans_Stream::map_nidpid_lock();
    SB_Trans::Sock_Stream *lp_stream =
      SB_Trans::Sock_Stream::create(la_name,
                                    pp_name,
                                    la_prog,
                                    pv_ic,
                                    lp_sock,
                                    NULL,
                                    NULL,
                                    &ms_ldone_cbt,
                                    &ms_abandon_cbt,
                                    &msg_mon_oc_cbt,
                                    gv_ms_lim_cb,
                                    &gv_ms_recv_q,
                                    &gv_ms_lim_q,
                                    &gv_ms_event_mgr,
                                    -1, // open nid
                                    -1, // open pid
#ifdef SQ_PHANDLE_VERIFIER
                                    -1, // open verif
#endif
                                    pv_nid,
                                    pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                    pv_verif,
#endif
                                    pv_ptype);
    lp_stream->start_stream();
    SB_Trans::Trans_Stream::map_nidpid_unlock();

    if (pv_reopen) {
        // close decremented, so put it back
        SB_Trans::Trans_Stream::add_stream_con_count(1);
    }

    // fill out od
    if (!pv_reopen)
        ms_od_map_name_add(pp_name, pp_od);
    strcpy(pp_od->ia_port, pp_msg->u.reply.u.open.port);
    strcpy(pp_od->ia_process_name, pp_name);
    pp_od->ip_stream = lp_stream;
    pp_od->iv_nid = pv_nid;
    pp_od->iv_pid = pv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    pp_od->iv_verif = pv_verif;
#endif
    ms_util_fill_phandle_name(pp_phandle,
                              pp_name,
                              pp_od->iv_nid,
                              pp_od->iv_pid
#ifdef SQ_PHANDLE_VERIFIER
                             ,pp_od->iv_verif
#endif
                             );
    ms_util_fill_phandle_oid(pp_phandle, pp_od->iv_oid);
    if (pp_oid != NULL)
        *pp_oid = pp_od->iv_oid;
    if (gv_ms_trace)
        trace_where_printf(pp_where, "phandle-to-oid oid=%d\n",
                           pp_od->iv_oid);
    if (gv_ms_trace_mon) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, pp_phandle);
        trace_where_printf(pp_where, "EXIT OK connected, pname=%s, sock=%d, oid=%d\n",
                           pp_name,
                           lp_sock->get_sock(),
                           pp_od->iv_oid);
        trace_where_printf(pp_where, "EXIT phandle=%s\n", la_phandle);
    }
    return XZFIL_ERR_OK;
}

SB_Export int msg_mon_open_process_nowait_cb(char                        *pp_name,
                                             SB_Phandle_Type             *pp_phandle,
                                             MS_Mon_Open_Process_Cb_Type  pv_callback,
                                             long long                    pv_tag,
                                             int                         *pp_done,
                                             int                         *pp_oid) {
    const char *WHERE = "msg_mon_open_process_nowait_cb";
    char        la_name[MS_MON_MAX_PROCESS_NAME+1];
    Ms_Od_Type *lp_od;
    int         lv_fserr;
    int         lv_status;
    SB_API_CTR (lv_zctr, MSG_MON_OPEN_PROCESS_NOWAIT_CB);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_OPEN_PROCESS_NOWAIT_CB, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=%s, phandle=%p, cb=%p, tag=0x%llx, done=%p, oid=%p\n",
                           pp_name, pfp(pp_phandle), SB_CB_TO_PTR(pv_callback),
                           pv_tag, pfp(pp_done), pfp(pp_oid));

    SB_util_get_case_insensitive_name(pp_name, la_name);

    lv_fserr = msg_mon_open_process_com_ph1(la_name,
                                            pp_phandle,
                                            NULL,
                                            false,  // reopen
                                            true,   // death notification
                                            false,  // backup
                                            false,  // fs
                                            &lp_od,
                                            pp_done);
    if (*pp_done) {
        if (lv_fserr == XZFIL_ERR_OK)
            *pp_oid = lp_od->iv_oid;
        return lv_fserr;
    }
    lv_status = gv_ms_open_sem.wait();
    SB_util_assert_ieq(lv_status, 0);
    Ms_Open_Thread *lp_thr =
      new Ms_Open_Thread(WHERE,
                         la_name,
                         pv_callback,
                         pv_tag,
                         false, // ic
                         lp_od);
    lp_thr->start();
    *pp_done = false;
    lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return lv_fserr;
}


//
// open 'self'
// similar to msg_mon_open_process
//
int msg_mon_open_self(char            *pp_name,
                      SB_Phandle_Type *pp_phandle,
                      int             *pp_oid,
                      bool             pv_ic) {
    const char *WHERE = "msg_mon_open_self";
    Ms_Od_Type *lp_od;
    int         lv_fserr;
    int         lv_ref;

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=<self>, phandle=%p, oid=%p, ic=%d\n",
                           pfp(pp_phandle), pfp(pp_oid), pv_ic);
    if (!gv_ms_calls_ok) // msg_mon_open_self
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    lp_od = ms_od_map_name_to_od(pp_name);
    if (lp_od == NULL) {
        lp_od = ms_od_alloc();
        if (lp_od == NULL) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE could not alloc od\n");
            return XZFIL_ERR_NOBUFSPACE;
        }
        ms_od_map_name_add(pp_name, lp_od);
        strcpy(lp_od->ia_process_name, pp_name);
        lp_od->iv_nid = -1;
        lp_od->iv_pid = -1;
#ifdef SQ_PHANDLE_VERIFIER
        lp_od->iv_verif = -1;
#endif
        lp_od->iv_self = true;
        SB_Buf_Line la_name;
        sprintf(la_name, "self (%s)", pp_name);
        strcpy(lp_od->ia_prog, ga_ms_su_prog);
        if (pv_ic)
            strcat(la_name, "-IC");
        lp_od->ip_stream =
          SB_Trans::Loop_Stream::create(la_name,
                                        pp_name,
                                        ga_ms_su_prog,
                                        pv_ic,
                                        &ms_ldone_cbt,
                                        &ms_abandon_cbt,
                                        &msg_mon_oc_cbt,
                                        gv_ms_lim_cb,
                                        &gv_ms_recv_q,
                                        &gv_ms_lim_q,
                                        &gv_ms_event_mgr);
        lv_ref = 1;
    } else
        lv_ref = ms_od_ref_add(lp_od);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "references=%d\n", lv_ref);
    if (lv_ref == 1) {
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "send inline open, references=%d\n",
                               lv_ref);
        // send open to remote
        SB_Trans::Stream_Base *lp_stream = lp_od->ip_stream;
        MS_Md_Type *lp_open_md;
        SB_Trans::Msg_Mgr::get_md(&lp_open_md, // msg_mon_open_self
                                  lp_stream,
                                  NULL,
                                  true,        // send
                                  NULL,        // fserr
                                  "open (msg_mon_open_self)",
                                  MD_STATE_SEND_INLINE_OPEN_SELF);
        SB_util_assert_pne(lp_open_md, NULL); // TODO: can't get md
        lp_open_md->iv_inline = true;
        lv_fserr = lp_stream->exec_open(lp_open_md,
                                        lp_open_md->iv_link.iv_id.i); // reqid
        if (lv_fserr == XZFIL_ERR_OK) {
            lp_stream->wait_req_done(lp_open_md);
            SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_REQ_DONE,
                               lp_open_md->iv_link.iv_id.i);
            if (gv_ms_trace)
                trace_where_printf(WHERE, "open reply-done=1\n");
            lv_fserr = lp_open_md->out.iv_fserr;
            if (lv_fserr != XZFIL_ERR_OK) {
                SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_EXIT, lv_fserr);
            }
        } else {
            SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_EXIT, lv_fserr);
        }
        MS_BUF_MGR_FREE(lp_open_md->out.ip_recv_ctrl);
        SB_Trans::Msg_Mgr::put_md(lp_open_md->iv_link.iv_id.i, "finish-send OPEN");
    }

    ms_util_fill_phandle_name(pp_phandle,
                              pp_name,
                              lp_od->iv_nid,
                              lp_od->iv_pid
#ifdef SQ_PHANDLE_VERIFIER
                             ,lp_od->iv_verif
#endif
                             );
    ms_util_fill_phandle_oid(pp_phandle, lp_od->iv_oid);
    if (pp_oid != NULL)
        *pp_oid = lp_od->iv_oid;
    if (gv_ms_trace_mon) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, pp_phandle);
        trace_where_printf(WHERE, "EXIT OK, pname=%s, oid=%d, phandle=%s\n",
                           pp_name,
                           lp_od->iv_oid,
                           la_phandle);
    }
    return XZFIL_ERR_OK;
}

//
// Purpose: is self?
//
bool msg_mon_is_self(SB_Phandle_Type *pp_phandle) {
    Ms_Od_Type *lp_od;

    lp_od = ms_od_map_phandle_to_od(pp_phandle);
    if (lp_od != NULL)
        return lp_od->iv_self;
    return false;
}





//
// Purpose: handle process close
//
SB_Export int msg_mon_process_close() {
    const char *WHERE = "msg_mon_process_close";
    int         lv_lerr;
    BMS_SRE     lv_sre;
    int         lv_status;
    SB_API_CTR (lv_zctr, MSG_MON_PROCESS_CLOSE);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_PROCESS_CLOSE, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER\n");
    if (!gv_ms_calls_ok) // msg_mon_process_close
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    do {
        lv_status = XWAIT(LREQ, -1);
        CHK_WAITIGNORE(lv_status);
        lv_lerr = BMSG_LISTEN_(reinterpret_cast<short *>(&lv_sre),   // sre
                               BLISTEN_ALLOW_IREQM,          // listenopts
                               0);                           // listenertag
    } while (lv_lerr == XSRETYPE_NOWORK);
    SB_util_assert_it(lv_sre.sre_flags & BSRE_MON); // sw fault
    BMSG_REPLY_(lv_sre.sre_msgId,    // msgid
                NULL,                // replyctrl
                0,                   // replyctrlsize
                NULL,                // replydata
                0,                   // replydatasize
                0,                   // errorclass
                NULL);               // newphandle
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT OK\n");
    return XZFIL_ERR_OK;
}

//
// Purpose: handle process shutdown
//
SB_Export int msg_mon_process_shutdown() {
    const char *WHERE = "msg_mon_process_shutdown";
    SB_API_CTR (lv_zctr, MSG_MON_PROCESS_SHUTDOWN);

    if (gv_ms_shutdown_fast)
        return msg_mon_process_shutdown_fast();
    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_PROCESS_SHUTDOWN, 0);
    return msg_mon_process_shutdown_ph1(WHERE,
                                        true,
                                        MSG_MON_PROCESS_SHUTDOWN_TYPE_REG);
}

//
// Purpose: handle process shutdown (fast)
// Where fast does a shutdown of threads, tells monitor of exit,
// but not all memory is recovered.
//
SB_Export int msg_mon_process_shutdown_fast() {
    const char *WHERE = "msg_mon_process_shutdown_fast";
    SB_API_CTR (lv_zctr, MSG_MON_PROCESS_SHUTDOWN_FAST);
    int         lv_ret;

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_PROCESS_SHUTDOWN_FAST, 0);
    lv_ret = msg_mon_process_shutdown_ph1(WHERE,
                                          false,
                                          MSG_MON_PROCESS_SHUTDOWN_TYPE_FAST);
    return lv_ret;
}

//
// Purpose: handle process shutdown (now)
//
SB_Export void msg_mon_process_shutdown_now() {
    const char *WHERE = "msg_mon_process_shutdown_now";
    SB_API_CTR (lv_zctr, MSG_MON_PROCESS_SHUTDOWN_NOW);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_PROCESS_SHUTDOWN_NOW, 0);
    // forget return code
    msg_mon_process_shutdown_ph1(WHERE,
                                 true,
                                 MSG_MON_PROCESS_SHUTDOWN_TYPE_NOW);
}

//
// Purpose: handle process shutdown
//
int msg_mon_process_shutdown_ph1(const char                    *pp_where,
                                 bool                           pv_finalize,
                                 Msg_Mon_Process_Shutdown_Type  pv_shutdown_type) {
    Mon_Msg_Type           *lp_msg;
    Ms_Od_Type             *lp_od;
    SB_Trans::Trans_Stream *lp_stream;
    int                     lv_fserr;
#ifdef USE_SB_SP_LOG
    SB_Buf_Lline            lv_line;
#endif
    int                     lv_max;
    int                     lv_mpierr;
    int                     lv_oid;
#ifdef USE_SB_SP_LOG
    int                     lv_status;
#endif

    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "ENTER\n");
    if (!gv_ms_calls_ok) // msg_mon_process_shutdown
        return ms_err_rtn_msg(pp_where, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (gv_ms_shutdown_called)
        return ms_err_rtn_msg(pp_where, "msg_mon_process_shutdown() already called",
                              XZFIL_ERR_INVALIDSTATE);

    sb_log_aggr_cap_log_flush();
#ifdef USE_SB_SP_LOG
#if 0 // permanently disabled
    // issue shutdown event (pid/etc set)
    sprintf(&lv_line, "shutdown - p-id=%d/%d, tid=%ld, pname=%s\n",
            gv_ms_su_nid,
            gv_ms_su_pid,
            SB_Thread::Sthr::self_id(),
            ga_ms_su_pname);
    lv_status = SB_log_write_str(SQEVL_SEABED, // USE_SB_SP_LOG
                                 SB_EVENT_ID,
                                 SQ_LOG_SEAQUEST,
                                 SQ_LOG_INFO,
                                 &lv_line);
    CHK_STATUSIGNORE(lv_status);
#endif
#endif

    gv_ms_shutdown_called = true;
    gv_ms_calls_ok = false;
    gv_ms_mon_calls_ok = false;

    // tell fs of shutdown
    ms_fs_shutdown_ph1();
    // tell pctl of shutdown
    pctl_shutdown();
    // tell streams we're in shutdown
    SB_Trans::Trans_Stream::set_shutdown(true);
    if (gv_ms_trans_sock)
        SB_Trans::Sock_Stream::shutdown();

    if (pv_shutdown_type != MSG_MON_PROCESS_SHUTDOWN_TYPE_FAST) {
        // close any open process
        gv_ms_od_mgr.free_entry(0);
        lv_max = static_cast<int>(gv_ms_od_mgr.get_cap());
        for (lv_oid = 1; lv_oid < lv_max; lv_oid++) {
            lp_od = gv_ms_od_mgr.get_entry(lv_oid);
            if (lp_od == NULL)
                continue;
            if ((lp_od->iv_inuse) && (lp_od->ip_stream != NULL)) {
                msg_mon_close_process_od(lp_od, true, false);
            }
        }

        // close accepted streams
        SB_Trans::Trans_Stream::close_nidpid_streams(false);

        if (gv_ms_trans_sock)
            SB_Trans::Sock_Stream::close_streams(false);
    }

    // shutdown ms
    ms_shutdown();

    // shutdown open-threads
    Ms_Open_Thread::shutdown();

    // tell fs of shutdown
    ms_fs_shutdown_ph2();

    // tell ic of shutdown
    ms_interceptor_shutdown();

    // We're about to tell the monitor to exit, stop completions
    lp_stream = SB_Trans::Trans_Stream::get_mon_stream();
    if (lp_stream != NULL) {
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "stop completions for %s\n",
                               lp_stream->get_name());
        lp_stream->stop_completions();
    }

    // shutdown event manager
    SB_Ms_Event_Mgr::shutdown();

    if (pv_shutdown_type == MSG_MON_PROCESS_SHUTDOWN_TYPE_NOW)
        lv_mpierr = MPI_SUCCESS;
    else {
        // send the exit to the monitor
        Mon_Msg_Auto lv_msg;
        lp_msg = &lv_msg;
        lp_msg->type = MsgType_Service;
        lp_msg->noreply = false;
        lp_msg->u.request.type = ReqType_Exit;
        lp_msg->u.request.u.exit.nid = gv_ms_su_nid;
        lp_msg->u.request.u.exit.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lp_msg->u.request.u.exit.verifier = gv_ms_su_verif;
        ms_util_string_clear(lp_msg->u.request.u.exit.process_name,
                             sizeof(lp_msg->u.request.u.exit.process_name));
#endif
#ifdef SQ_PHANDLE_VERIFIER
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "send exit req to mon, p-id=%d/%d/" PFVY "\n",
                               gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif);
#else
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "send exit req to mon, p-id=%d/%d\n",
                               gv_ms_su_nid, gv_ms_su_pid);
#endif
        lv_mpierr = msg_mon_sendrecv_mon(pp_where,
                                         "exit",
                                         lp_msg,
                                         lv_msg.get_error());
        if (msg_mon_msg_ok(pp_where,
                           "exit req",
                           &lv_mpierr,
                           lp_msg,
                           MsgType_Service,
                           ReplyType_Generic)) {
            lv_mpierr = lp_msg->u.reply.u.generic.return_code;
            if (gv_ms_trace_mon) {
                if (lv_mpierr == MPI_SUCCESS)
                    trace_where_printf(pp_where, "EXIT OK exit req, p-id=%d/%d, pname=%s\n",
                                       lp_msg->u.reply.u.generic.nid,
                                       lp_msg->u.reply.u.generic.pid,
                                       lp_msg->u.reply.u.generic.process_name);
                else
                    trace_where_printf(pp_where, "EXIT FAILED exit req, ret=%d\n",
                                       lv_mpierr);
            }
        }
    }

    // stop local-io thread
    delete gp_local_mon_io;
    gp_local_mon_io = NULL;

    // we just told the monitor to exit - expects a disconnect here
    lp_stream = SB_Trans::Trans_Stream::get_mon_stream();
    if (lp_stream != NULL) {
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "closing stream for %s\n",
                               lp_stream->get_name());
        SB_Trans::Trans_Stream::close_stream(pp_where,
                                             lp_stream,
                                             false,
                                             true,
                                             false);
    }

    // cleanup misc memory
    msg_mon_cleanup_mem();

#if 0
    msg_mon_display_tags();
#endif

    if (pv_shutdown_type != MSG_MON_PROCESS_SHUTDOWN_TYPE_FAST) {
        if (gp_ms_rsvd_md != NULL)
            SB_Trans::Msg_Mgr::put_md(gp_ms_rsvd_md->iv_msgid, "finish-rsvd-md");
        if (gv_ms_trace_md) {
            if (SB_Trans::Msg_Mgr::trace_inuse_md_count() > 0) {
                trace_where_printf(pp_where, "WARNING MDs still inuse\n");
                SB_Trans::Msg_Mgr::trace_inuse_mds();
            }
        }

        // delete streams that might be delete-pending
        SB_Trans::Trans_Stream::delete_streams(false);
    }

    // If MPI_Error_class is called, it needs to be called before MPI_Finalize
    lv_fserr = ms_err_mpi_rtn_msg_noassert(pp_where, "EXIT", lv_mpierr);
    if (pv_finalize) {
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "about to call finalize\n");
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "finalize finished\n");
    }
    if (gv_ms_trace_stats)
        ms_stats(NULL, true);

    return lv_fserr;
}

//
// Purpose: handle process startup
//
SB_Export int msg_mon_process_startup(int pv_sysmsgs)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, MSG_MON_PROCESS_STARTUP);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_PROCESS_STARTUP, 0);
    return msg_mon_process_startup_com(pv_sysmsgs,
                                       gv_ms_attach,
                                       true,   // eventmsgs
                                       true,   // pipeio
                                       false); // altsig
}

//
// Purpose: handle process startup
//
SB_Export int msg_mon_process_startup2(int pv_sysmsgs, int pv_eventmsgs)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, MSG_MON_PROCESS_STARTUP2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_PROCESS_STARTUP2, 0);
    return msg_mon_process_startup_com(pv_sysmsgs,
                                       gv_ms_attach,
                                       pv_eventmsgs,
                                       true,   // pipeio
                                       false); // altsig
}

//
// Purpose: handle process startup
//
SB_Export int msg_mon_process_startup3(int pv_sysmsgs, int pv_pipeio, bool pv_remap_stderr)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, MSG_MON_PROCESS_STARTUP3);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_PROCESS_STARTUP3, 0);
    return msg_mon_process_startup_com(pv_sysmsgs,
                                       gv_ms_attach,
                                       true,       // eventmsgs
                                       pv_pipeio,
                                       false,
                                       pv_remap_stderr);     // altsig
}

//
// Purpose: handle process startup
//
SB_Export int msg_mon_process_startup4(int pv_sysmsgs, int pv_pipeio, int pv_altsig)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, MSG_MON_PROCESS_STARTUP4);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_PROCESS_STARTUP4, 0);
    return msg_mon_process_startup_com(pv_sysmsgs,
                                       gv_ms_attach,
                                       true,       // eventmsgs
                                       pv_pipeio,
                                       pv_altsig);
}

//
// Purpose: handle process startup
//
int msg_mon_process_startup_com(bool pv_sysmsgs,
                                bool pv_attach,
                                bool pv_eventmsgs,
                                bool pv_pipeio,
                                bool pv_altsig,
                                bool pv_remap_stderr)
SB_THROWS_FATAL {
    const char   *WHERE = "msg_mon_process_startup";
    char         *lp_s;
    SB_Buf_Lline  lv_cmdline;
    int           lv_ppid;

    if (gv_ms_trace_mon) {
        lv_ppid = getppid();
        lp_s = SB_util_get_cmdline(lv_ppid,
                                   true, // args
                                   &lv_cmdline,
                                   lv_cmdline.size());
        if (lp_s == NULL)
            lp_s = const_cast<char *>("<unknown>");
        trace_where_printf(WHERE, "ENTER sysmsgs=%d, attach=%d, eventmsgs=%d, pipeio=%d, altsig=%d, ppid=%d, pcmdline=%s\n",
                           pv_sysmsgs, pv_attach, pv_eventmsgs, pv_pipeio, pv_altsig, lv_ppid, lp_s);
    }
    if (!gv_ms_calls_ok) // msg_mon_process_startup
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (gv_ms_su_called)
        return ms_err_rtn_msg(WHERE, "msg_mon_process_startup() already called",
                              XZFIL_ERR_INVALIDSTATE);

    gv_ms_su_called = true;
    gv_ms_su_sysmsgs = pv_sysmsgs;
    gv_ms_su_eventmsgs = pv_eventmsgs;
    gv_ms_su_pipeio = pv_pipeio;
    gv_ms_su_altsig = pv_altsig;

    int lv_fserr = msg_mon_process_startup_ph1(pv_attach, pv_altsig, pv_remap_stderr);
    if (gp_local_mon_io != NULL)
        gv_ms_mon_calls_ok = true;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return lv_fserr;
}

//
// Purpose: handle process startup
//
int msg_mon_process_startup_ph1(bool pv_attach, bool pv_altsig, bool pv_remap_stderr)
SB_THROWS_FATAL {
    const char           *WHERE = "msg_mon_process_startup_ph1";
    Mon_Shared_Msg_Type  *lp_msg;
#ifdef OFED_MUTEX
    char                  la_sem_name[MAX_PROCESS_PATH];
    sem_t                *lp_sem;
#endif
    int                   lv_fserr;
    int                   lv_mpierr;
    bool                  lv_ret;

    if (gv_ms_trans_sock) {
        block_lio_signals(); // local-io setup before SB threads created
        int lv_serr = SB_Trans::Sock_Stream::open_port(ga_ms_su_sock_a_port);
        if (lv_serr)
            return ms_err_sock_rtn_msg_fatal(WHERE, "process-startup: Sock::open_port() failed",
                                        lv_serr);
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "accept sock port=%s\n",
                               ga_ms_su_sock_a_port);
        gp_ms_su_sock_stream = SB_Trans::Sock_Stream::create();
        gp_ms_su_sock_stream->start_stream();
    }


#ifdef OFED_MUTEX
    sprintf(la_sem_name, "/monitor.sem2.%s", getenv("USER"));
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "opening semaphore=%s\n", la_sem_name);
    lp_sem = sem_open(la_sem_name, 0, 0644, 0);
    if (lp_sem == SEM_FAILED) {
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "Can't open semaphore=%s, errno=%d\n",
                               la_sem_name, errno);
        sem_close(lp_sem);
        // TODO should be fatal
        SB_util_assert_pne(lp_sem, SEM_FAILED);
    }
    sem_post(lp_sem);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "closing semaphore=%s\n", la_sem_name);
    sem_close(lp_sem);
#endif

    if (!gv_ms_trans_sock)
        block_lio_signals(); // local-io setup before SB threads created

    // need to start monitor stream before doing local-i/o
    SB_Trans::Sock_Stream *lp_stream =
      SB_Trans::Sock_Stream::create("monitor",
                                    "monitor",
                                    "<none>",  // prog
                                    false, // ic
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    -1,  // open nid
                                    -1,  // open pid
#ifdef SQ_PHANDLE_VERIFIER
                                    -1,  // open verif
#endif
                                    -1,  // opened nid
                                    -1,  // opened pid
#ifdef SQ_PHANDLE_VERIFIER
                                    -1,  // opened verif
#endif
                                    -1); // opened type
    lv_fserr = lp_stream->start_stream();
    if (lv_fserr != XZFIL_ERR_OK)
        return ms_err_rtn_msg_fatal(WHERE,
                                    "process-startup: stream start failed",
                                    static_cast<short>(lv_fserr),
                                    true);

    if (gv_ms_trace_locio) {
        Local_IO_To_Monitor::cv_trace = true;
        Local_IO_To_Monitor::cp_trace_cb = trace_where_vprintf;
    }
    gp_local_mon_io = new Local_IO_To_Monitor(gv_ms_su_pid);

    if ((gp_local_mon_io != NULL) && !gp_local_mon_io->init_comm(pv_altsig)) {
        delete gp_local_mon_io;
        gp_local_mon_io = NULL;
    }
    SB_util_assert_pne(gp_local_mon_io, NULL);
    lv_ret = gp_local_mon_io->set_cb(msg_mon_recv_msg_loc_cbt, "recv");
    SB_util_assert_if(lv_ret);
    lv_ret = gp_local_mon_io->set_cb(msg_mon_recv_notice_msg_loc_cbt, "notice");
    SB_util_assert_if(lv_ret);


    Mon_Shared_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->msg.type = MsgType_Service;
    lp_msg->msg.u.request.type = ReqType_Startup;
    if (pv_attach) {
        lp_msg->msg.noreply = false;
        lp_msg->msg.u.request.u.startup.nid = -1;
        lp_msg->msg.u.request.u.startup.pid = -1;
#ifdef SQ_PHANDLE_VERIFIER
        lp_msg->msg.u.request.u.startup.verifier = -1;
#endif
    } else {
        lp_msg->msg.noreply = true;
        lp_msg->msg.u.request.u.startup.nid = gv_ms_su_nid;
        lp_msg->msg.u.request.u.startup.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lp_msg->msg.u.request.u.startup.verifier = gv_ms_su_verif;
#endif
    }
    lp_msg->msg.u.request.u.startup.os_pid = getpid();
    ms_util_string_copy(lp_msg->msg.u.request.u.startup.process_name,
                        sizeof(lp_msg->msg.u.request.u.startup.process_name),
                        ga_ms_su_pname);
    if (gv_ms_trans_sock)
        ms_util_string_copy(lp_msg->msg.u.request.u.startup.port_name,
                            sizeof(lp_msg->msg.u.request.u.startup.port_name),
                            ga_ms_su_sock_a_port);
    SB_Buf_Lline lv_cmdline;
    char *lp_s = SB_util_get_cmdline(0,
                                     false, // args
                                     &lv_cmdline,
                                     lv_cmdline.size());
    if (lp_s == NULL)
        lp_s = const_cast<char *>("<unknown>");
    ms_util_string_copy(lp_msg->msg.u.request.u.startup.program,
                        sizeof(lp_msg->msg.u.request.u.startup.program),
                        lp_s);
    lp_msg->msg.u.request.u.startup.event_messages = gv_ms_su_eventmsgs;
    lp_msg->msg.u.request.u.startup.system_messages = gv_ms_su_sysmsgs;
    lp_msg->msg.u.request.u.startup.paired = false;
    lp_msg->msg.u.request.u.startup.startup_size = sizeof(lp_msg->msg.u.request.u.startup);
#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send startup request to mon, attach=%d, p-id=%d/%d/" PFVY ", pid=%d, pname=%s, port=%s, program=%s, sysmsgs=%d, eventmsgs=%d, startup-size=%d\n",
                           pv_attach,
                           lp_msg->msg.u.request.u.startup.nid,
                           lp_msg->msg.u.request.u.startup.pid,
                           lp_msg->msg.u.request.u.startup.verifier,
                           lp_msg->msg.u.request.u.startup.os_pid,
                           ga_ms_su_pname,
                           lp_msg->msg.u.request.u.startup.port_name,
                           lp_s,
                           lp_msg->msg.u.request.u.startup.system_messages,
                           lp_msg->msg.u.request.u.startup.event_messages,
                           lp_msg->msg.u.request.u.startup.startup_size);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send startup request to mon, attach=%d, p-id=%d/%d, pid=%d, pname=%s, port=%s, program=%s, sysmsgs=%d, eventmsgs=%d, startup-size=%d\n",
                           pv_attach,
                           lp_msg->msg.u.request.u.startup.nid,
                           lp_msg->msg.u.request.u.startup.pid,
                           lp_msg->msg.u.request.u.startup.os_pid,
                           ga_ms_su_pname,
                           lp_msg->msg.u.request.u.startup.port_name,
                           lp_s,
                           lp_msg->msg.u.request.u.startup.system_messages,
                           lp_msg->msg.u.request.u.startup.event_messages,
                           lp_msg->msg.u.request.u.startup.startup_size);
#endif
    if (pv_attach) {
        lp_msg->trailer.attaching = true;
        lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                         "startup-attach",
                                         &lp_msg->msg,
                                         lv_msg.get_error());
        if (msg_mon_msg_ok(WHERE,
                           "startup req",
                           &lv_mpierr,
                           &lp_msg->msg,
                           MsgType_Service,
                           ReplyType_Startup)) {
            lv_mpierr = lp_msg->msg.u.reply.u.startup_info.return_code;
            if (lv_mpierr == MPI_SUCCESS) {
#ifdef SQ_PHANDLE_VERIFIER
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "startup-attach req, p-id=%d/%d/" PFVY ", pname=%s\n",
                                       lp_msg->msg.u.reply.u.startup_info.nid,
                                       lp_msg->msg.u.reply.u.startup_info.pid,
                                       lp_msg->msg.u.reply.u.startup_info.verifier,
                                       lp_msg->msg.u.reply.u.startup_info.process_name);
#else
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "startup-attach req, p-id=%d/%d, pname=%s\n",
                                       lp_msg->msg.u.reply.u.startup_info.nid,
                                       lp_msg->msg.u.reply.u.startup_info.pid,
                                       lp_msg->msg.u.reply.u.startup_info.process_name);
#endif
                gv_ms_su_nid = lp_msg->msg.u.reply.u.startup_info.nid;
                gv_ms_su_pid = lp_msg->msg.u.reply.u.startup_info.pid;
#ifdef SQ_PHANDLE_VERIFIER
                gv_ms_su_verif = lp_msg->msg.u.reply.u.startup_info.verifier;
#endif


                lp_msg->trailer.attaching = false;
                gp_local_mon_io->iv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
                gp_local_mon_io->iv_verifier = gv_ms_su_verif;
#endif
                strcpy(ga_ms_su_pname,
                       lp_msg->msg.u.reply.u.startup_info.process_name);
#ifdef SQ_PHANDLE_VERIFIER
                sprintf(ga_ms_su_pname_seq, "%s:" PFVY, ga_ms_su_pname, gv_ms_su_verif);
#endif
                ms_util_fill_phandle_name(&gv_ms_su_phandle,
                                          ga_ms_su_pname,
                                          gv_ms_su_nid,
                                          gv_ms_su_pid
#ifdef SQ_PHANDLE_VERIFIER
                                         ,gv_ms_su_verif
#endif
                                         );
                if (pv_attach) {
                    // Connect to monitor via pipes and remap stdout and stderr
                    if (gv_ms_su_pipeio)
                        ms_fifo_setup(1, lp_msg->msg.u.reply.u.startup_info.fifo_stdout);
                    ms_fifo_setup(2, lp_msg->msg.u.reply.u.startup_info.fifo_stderr, pv_remap_stderr);
                }
                if (gv_ms_trace_name) {
                    sprintf(ga_ms_su_trace_pname, "%s:%d/%d",
                            ga_ms_su_pname, gv_ms_su_nid, gv_ms_su_pid);
                    trace_set_pname(ga_ms_su_trace_pname);
                }
                if (gv_ms_trace_enable)
                    trace_where_printf(WHERE, "pname=%s, nid=%d, pid=%d\n",
                                       ga_ms_su_pname,
                                       gv_ms_su_nid,
                                       gv_ms_su_pid);
            }
        }
        if (lv_mpierr != MPI_SUCCESS)
            return ms_err_mpi_rtn_msg_fatal(WHERE, "process-startup: error returned by monitor",
                                            lv_mpierr);
    } else {
        lv_mpierr = lv_msg.get_error();
        if (lv_mpierr == MPI_SUCCESS) {
            lp_msg->msg.reply_tag = 0;
            if (gp_local_mon_io->send(&lp_msg->msg)) {
                lv_mpierr = ms_err_errno_to_mpierr(WHERE);
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_SEND,
                                   errno,
                                   lv_mpierr);
            } else
                lv_mpierr = MPI_SUCCESS;
        }
        if (lv_mpierr != MPI_SUCCESS)
            return ms_err_mpi_rtn_msg_fatal(WHERE, "process-startup: local/io-send() failed",
                                            lv_mpierr);
    }
    if (pv_attach)
        lv_msg.del_msg();
    else
        lv_msg.forget_msg();

    return ms_err_rtn_msg(WHERE,
                          "process-startup: ok",
                          static_cast<short>(lv_fserr));
}

void msg_mon_recv_msg(MS_Md_Type *pp_md) {
    const char *WHERE = "msg_mon_recv_msg";

    Mon_Msg_Type *lp_msg = reinterpret_cast<Mon_Msg_Type *>(pp_md->out.ip_recv_data);
    if (gv_ms_trace_mon) {
        const char *lp_msg_type = msg_util_get_msg_type(lp_msg->type);
        trace_where_printf(WHERE, "receiving %s (%d) mon message\n",
                           lp_msg_type, lp_msg->type);
        msg_mon_trace_msg(WHERE, lp_msg);
    }
    switch (lp_msg->type) {
    case MsgType_Change:
        msg_mon_recv_msg_change(lp_msg);
        break;
    case MsgType_Close:
        break;
    case MsgType_NodeDown:
        msg_mon_recv_msg_node_down(lp_msg);
        break;
    case MsgType_NodeQuiesce:
        msg_mon_recv_msg_node_quiesce(lp_msg);
        break;
    case MsgType_NodeUp:
        msg_mon_recv_msg_node_up(lp_msg);
        break;
    case MsgType_Open:
        break;
    case MsgType_ProcessCreated:
        msg_mon_recv_msg_process_created(lp_msg);
        break;
    case MsgType_ProcessDeath:
        // message already processed
        break;
    case MsgType_Shutdown:
        msg_mon_recv_msg_shutdown(lp_msg);
        break;
    default:
        msg_mon_recv_msg_unknown(lp_msg);
        break;
    }
}

//
// callback-target
//
void msg_mon_recv_msg_cbt(MS_Md_Type *pp_md) {
    const char           *WHERE = "msg_mon_recv_msg_cbt";
    char                  la_name_seq[50];
    char                 *lp_key;
    Mon_Msg_Type         *lp_msg;
    Ms_Od_Type           *lp_od;
    char                 *lp_value;
    MS_Mon_Trace_Cb_Type  lv_cb;
    bool                  lv_discard;
    int                   lv_inx;
    int                   lv_status;
    bool                  lv_stream_is_mon_stream;

    if (gv_ms_msg_timestamp)
        gettimeofday(&pp_md->iv_ts_msg_srv_rcvd, NULL);
    lp_msg = reinterpret_cast<Mon_Msg_Type *>(pp_md->out.ip_recv_data);
    if (gv_ms_trace_mon) {
        const char *lp_msg_type = msg_util_get_msg_type(lp_msg->type);
        trace_where_printf(WHERE,
                           "May add %s (%d) mon md (msgid=%d, md=%p) to server recv queue\n",
                           lp_msg_type, lp_msg->type,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));
        msg_mon_trace_msg(WHERE, lp_msg);
    }

    if (!gv_ms_shutdown_called && (lp_msg->type == MsgType_Open)) {
        // TODO: temporary - remove when monitor doesn't send msg
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "discarding mon md - mon open\n");
        MS_BUF_MGR_FREE(pp_md->out.ip_recv_data);
        pp_md->out.ip_recv_data = NULL;
        SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "recv-callback-open");
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXITING\n");
        return;
    }

    if (!gv_ms_shutdown_called && (lp_msg->type == MsgType_ProcessDeath)) {
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "delegating mon md - mon death\n");

        // if there's a closed od, close the stream on death
        lv_status = gv_ms_close_process_mutex.lock();
        SB_util_assert_ieq(lv_status, 0);
        lp_od = ms_od_map_name_to_od(lp_msg->u.request.u.death.process_name);
#ifdef SQ_PHANDLE_VERIFIER
        if (lp_od == NULL) {
            msg_mon_create_name_seq(lp_msg->u.request.u.death.process_name,
                                    lp_msg->u.request.u.death.verifier,
                                    la_name_seq,
                                    static_cast<int>(sizeof(la_name_seq)));
            lp_od = ms_od_map_name_to_od(la_name_seq);
        }
#endif
        if (lp_od != NULL) {
            if (lp_od->iv_fs_open &&
                lp_od->iv_fs_closed &&
                (lp_od->iv_ref_count == 0)) {
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "no opener, close od\n");
                // no opener, close od
                msg_mon_close_process_od(lp_od, true, false);
            }
        }
        lv_status = gv_ms_close_process_mutex.unlock();
        SB_util_assert_ieq(lv_status, 0);
        if (gv_ms_recv_q_proc_death)
            ms_recv_q_proc_death(lp_msg->u.request.u.death.nid,
                                 lp_msg->u.request.u.death.pid,
#ifdef SQ_PHANDLE_VERIFIER
                                 lp_msg->u.request.u.death.verifier,
#endif
                                 false,
                                 NULL);


        //
        // Be sure to post the death message to the receive queue
        //

        do {
            lv_status = SB_Trans::Trans_Stream::map_nidpid_trylock();
            if (lv_status == EBUSY)
                usleep(100);
        } while (lv_status);
        SB_util_assert_ieq(lv_status, 0);

        SB_Trans::Trans_Stream *lp_stream =
          SB_Trans::Trans_Stream::map_nidpid_to_stream(lp_msg->u.request.u.death.nid,
                                                       lp_msg->u.request.u.death.pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                       lp_msg->u.request.u.death.verifier,
#endif
                                                       false);

        if (lp_stream == NULL) {
            lp_stream = SB_Trans::Trans_Stream::get_mon_stream();
            lv_stream_is_mon_stream = true;
        } else
            lv_stream_is_mon_stream = false;

        if (lp_stream != NULL && lp_stream->ref_get() > 0) {
            lp_stream->post_mon_mutex_lock();

            if (lp_stream->post_mon_messages_get()) {

                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "death inside processing stream %s - post monitor message\n", lp_stream->get_name());

                if ((gv_ms_lim_cb != NULL) &&
                    gv_ms_lim_cb(0, // mon-msg
                                 NULL,
                                 0,
                                 pp_md->out.ip_recv_data,
                                 pp_md->out.iv_recv_data_size)) {
                    gv_ms_lim_q.add(&pp_md->iv_link);
                    gv_ms_event_mgr.set_event_all(INTR);
                } else {
                    gv_ms_recv_q.add(&pp_md->iv_link);
                    gv_ms_event_mgr.set_event_all(LREQ);
                }

                //
                // If this is not the monitor stream,
                // then set the post monitor flag to false.
                //
                if (!lv_stream_is_mon_stream)
                    lp_stream->post_mon_messages_set(false);
            } else {
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "discarding mon close md\n");
                ms_free_recv_bufs(pp_md);
                SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "close-md-free");
            }

            lp_stream->post_mon_mutex_unlock();

        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "discarding mon close md\n");
            ms_free_recv_bufs(pp_md);
            SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "close-md-free");
        }

        SB_Trans::Trans_Stream::map_nidpid_unlock();

        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXITING\n");
        return;
    }

    // Check for sb change
    lv_discard = false;
    if (lp_msg->type == MsgType_Change) {
        lp_key = lp_msg->u.request.u.change.key;
        lp_value = lp_msg->u.request.u.change.value;
        if (tolower(lp_key[0]) == 's') {
            if (strcasecmp(lp_key, "sbstats") == 0) {
                // key==sbstats: dump stats
                lv_discard = true;
                ms_stats(lp_value, false);
                if (gv_ms_trace_stats) // put it in trace file too
                    ms_stats(lp_value, true);
            } else if (strncasecmp(lp_key, "sbtrace-", 8) == 0) {
                // key==sbtrace: manipulate trace
                lv_discard = true;
                msg_mon_fs_trace_change(&lp_key[8], lp_value);
            } else if (strncasecmp(lp_key, "sqtrace-", 8) == 0) {
                // key==sqtrace: call registered callbacks
                lv_discard = true;
                for (lv_inx = 0; lv_inx < gv_ms_trace_callback_inx; lv_inx++) {
                    lv_cb = ga_ms_trace_callback[lv_inx];
                    lv_cb(&lp_key[8], lp_value);
                }
            }
        }
    }

    //
    // If NewProcess notice, convert return_code to ferr
    //
    if (lp_msg->type == MsgType_ProcessCreated) {
        lp_msg->u.request.u.process_created.return_code =
          ms_err_mpi_rtn_msg(WHERE,
                             "",
                             lp_msg->u.request.u.process_created.return_code);
        Ms_TC_Node *lp_node =
          static_cast<Ms_TC_Node *>
            (gv_ms_tc_map.get(lp_msg->u.request.u.process_created.tag));
        if (lp_node != NULL) {
            SB_Phandle_Type lv_phandle;
            gv_ms_tc_map.remove(lp_msg->u.request.u.process_created.tag); // free tag
            ms_util_fill_phandle_name(&lv_phandle,
                                      lp_msg->u.request.u.process_created.process_name,
                                      lp_msg->u.request.u.process_created.nid,
                                      lp_msg->u.request.u.process_created.pid
#ifdef SQ_PHANDLE_VERIFIER
                                     ,lp_msg->u.request.u.process_created.verifier
#endif
                                     );
            if (gv_ms_trace_mon) {
                char la_phandle[MSG_UTIL_PHANDLE_LEN];
                msg_util_format_phandle(la_phandle, &lv_phandle);
                trace_where_printf(WHERE, "process-created, doing callback, phandle=%s\n", la_phandle);
            }
            lp_node->iv_callback(&lv_phandle,
                                 reinterpret_cast<MS_Mon_NewProcess_Notice_def *>(&lp_msg->u.request.u.process_created));
            delete lp_node;
            lv_discard = true;
        }
    }

    //
    // discard duplicate close
    //
    if (lp_msg->type == MsgType_Close) {
        if (!msg_mon_conn_state_change(lp_msg->u.request.u.close.nid,
                                       lp_msg->u.request.u.close.pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       lp_msg->u.request.u.close.verifier,
#endif
                                       lp_msg->type)) {
            lv_discard = true;
        }
    }

    //
    // handle quiesce
    //
    if (lp_msg->type == MsgType_NodeQuiesce) {
        if ((gv_ms_lim_cb != NULL) &&
            gv_ms_lim_cb(0, // mon-msg
                         NULL,
                         0,
                         pp_md->out.ip_recv_data,
                         pp_md->out.iv_recv_data_size)) {
            gv_ms_lim_q.add_at_front(&pp_md->iv_link);
            gv_ms_event_mgr.set_event_all(INTR);
        } else {
            gv_ms_recv_q.add_at_front(&pp_md->iv_link);
            gv_ms_event_mgr.set_event_all(LREQ);
        }
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXITING\n");
        return;
    }

    //
    // discard notices during shutdown
    //
    if (gv_ms_shutdown_called)
        lv_discard = true;

    if (lv_discard) {
        // discard sb changes
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "discarding mon md - seabed msg\n");
        MS_BUF_MGR_FREE(pp_md->out.ip_recv_data);
        pp_md->out.ip_recv_data = NULL;
        SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "discard seabed mon msg");
    } else {
        if ((gv_ms_lim_cb != NULL) &&
            gv_ms_lim_cb(0, // mon-msg
                         NULL,
                         0,
                         pp_md->out.ip_recv_data,
                         pp_md->out.iv_recv_data_size)) {
            gv_ms_lim_q.add(&pp_md->iv_link);
            gv_ms_event_mgr.set_event_all(INTR);
        } else {
            gv_ms_recv_q.add(&pp_md->iv_link);
            gv_ms_event_mgr.set_event_all(LREQ);
        }
    }
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "EXITING\n");
}

void msg_mon_recv_msg_cbt_discard(const char *pp_where,
                                  MS_Md_Type *pp_md,
                                  bool        pv_ic) {
    Mon_Msg_Type *lp_msg;

    msg_mon_recv_msg(pp_md);
    lp_msg = reinterpret_cast<Mon_Msg_Type *>(pp_md->out.ip_recv_data);
    if (gv_ms_trace_mon) {
        const char *lp_msg_type = msg_util_get_msg_type(lp_msg->type);
        if (pv_ic)
            trace_where_printf(pp_where,
                               "IC stream, deleting mon message %s (%d), msgid=%d\n",
                               lp_msg_type, lp_msg->type,
                               pp_md->iv_link.iv_id.i);
        else
            trace_where_printf(pp_where,
                               "disabled mon-messages, deleting mon message %s (%d), msgid=%d\n",
                               lp_msg_type, lp_msg->type,
                               pp_md->iv_link.iv_id.i);
        msg_mon_trace_msg(pp_where, lp_msg);
    }
    MS_BUF_MGR_FREE(pp_md->out.ip_recv_data);
    pp_md->out.ip_recv_data = NULL;
    SB_Trans::Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "mon-msg disabled");
}

void msg_mon_recv_msg_change(Mon_Msg_Type *pp_msg) {
    pp_msg = pp_msg; // touch
}

void msg_mon_recv_msg_close(Mon_Msg_Type *pp_msg) {
    const char *WHERE = "msg_mon_recv_msg_close";

    if (gv_ms_trace_mon) {
        const char *lp_msg_type = msg_util_get_msg_type(pp_msg->type);
        trace_where_printf(WHERE, "receiving %s (%d) mon message\n",
                           lp_msg_type, pp_msg->type);
    }

    msg_mon_recv_msg_close_or_death(pp_msg->u.request.u.close.nid,
                                    pp_msg->u.request.u.close.pid,
#ifdef SQ_PHANDLE_VERIFIER
                                    pp_msg->u.request.u.close.verifier,
#endif
                                    pp_msg->u.request.u.close.process_name,
                                    pp_msg->u.request.u.close.aborted,
                                    true);

}

// Common routine to handle close and death
// stream processing
void msg_mon_recv_msg_close_or_death(int            pv_nid,
                                     int            pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                     SB_Verif_Type  pv_verif,
#endif
                                     char          *pp_process_name,
                                     int            pv_aborted,
                                     bool           pv_close) {
    const char *WHERE = "msg_mon_recv_msg_close_or_death";

    if (gv_ms_trace_mon) {
        if (pv_close)
#ifdef SQ_PHANDLE_VERIFIER
            trace_where_printf(WHERE, "close for p-id=%d/%d/" PFVY " (%s), abort=%d\n",
#else
            trace_where_printf(WHERE, "close for p-id=%d/%d (%s), abort=%d\n",
#endif
                               pv_nid,
                               pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                               pv_verif,
#endif
                               pp_process_name,
                               pv_aborted);
        else
#ifdef SQ_PHANDLE_VERIFIER
            trace_where_printf(WHERE, "death for p-id=%d/%d/" PFVY " (%s), abort=%d\n",
#else
            trace_where_printf(WHERE, "death for p-id=%d/%d (%s), abort=%d\n",
#endif
                               pv_nid,
                               pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                               pv_verif,
#endif
                               pp_process_name,
                               pv_aborted);
    }

    SB_Trans::Trans_Stream::map_nidpid_lock();

#ifdef SQ_PHANDLE_VERIFIER
    SB_Trans::Trans_Stream *lp_stream =
      SB_Trans::Trans_Stream::map_nidpid_to_stream(pv_nid, pv_pid, pv_verif, false);
#else
    SB_Trans::Trans_Stream *lp_stream =
      SB_Trans::Trans_Stream::map_nidpid_to_stream(pv_nid, pv_pid, false);
#endif

    if (lp_stream != NULL) {
        if (gv_ms_trace_mon || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID)) {
            if (pv_close)
#ifdef SQ_PHANDLE_VERIFIER
                trace_where_printf(WHERE, "close closing stream for p-id=%d/%d/" PFVY " (%s), using stream=%s\n",
#else
                trace_where_printf(WHERE, "close closing stream for p-id=%d/%d (%s), using stream=%s\n",
#endif
                                   pv_nid,
                                   pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                   pv_verif,
#endif
                                   pp_process_name,
                                   lp_stream->get_name());
            else
#ifdef SQ_PHANDLE_VERIFIER
                trace_where_printf(WHERE, "death closing stream for p-id=%d/%d/" PFVY " (%s), using stream=%s\n",
#else
                trace_where_printf(WHERE, "death closing stream for p-id=%d/%d (%s), using stream=%s\n",
#endif
                                   pv_nid,
                                   pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                   pv_verif,
#endif
                                   pp_process_name,
                                   lp_stream->get_name());
        }

        // if stream is in use, clear it
        Ms_Od_Type *lp_od = ms_od_map_oid_to_od(lp_stream->get_oid());
        if ((lp_od != NULL) &&
            (lp_od->iv_inuse) &&
            (lp_od->ip_stream == lp_stream))
            lp_od->ip_stream = NULL;
        lp_stream->map_nidpid_remove(false);
        SB_Trans::Trans_Stream::map_nidpid_unlock();
        SB_Trans::Trans_Stream::close_stream(WHERE,
                                             lp_stream,
                                             pv_aborted,
                                             true,
                                             false);
    } else
        SB_Trans::Trans_Stream::map_nidpid_unlock();

    if (gv_ms_trace_mon || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(WHERE, "completing close_or_death for p-id=%d/%d/" PFVY " (%s)\n",
#else
        trace_where_printf(WHERE, "completing close_or_death for p-id=%d/%d (%s)\n",
#endif
                           pv_nid,
                           pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                           pv_verif,
#endif
                           pp_process_name);
}

void msg_mon_recv_msg_loc_cbt(Mon_Msg_Type *pp_msg, int pv_size) {
    const char *WHERE = "msg_mon_recv_msg_loc_cbt";
    MS_Md_Type   *lp_md;
    Mon_Msg_Type *lp_msg;

    lp_md = msg_mon_loc_get_md(pp_msg, pv_size);
    lp_msg = reinterpret_cast<Mon_Msg_Type *>(lp_md->out.ip_recv_data);
    if (gv_ms_enable_messages || (lp_msg->type == MsgType_ProcessCreated))
        msg_mon_recv_msg_cbt(lp_md);
    else
        msg_mon_recv_msg_cbt_discard(WHERE, lp_md, false);
}

void msg_mon_recv_msg_node_down(Mon_Msg_Type *pp_msg) {
    pp_msg = pp_msg; // touch
}

void msg_mon_recv_msg_node_quiesce(Mon_Msg_Type *pp_msg) {
    pp_msg = pp_msg; // touch
}

void msg_mon_recv_msg_node_up(Mon_Msg_Type *pp_msg) {
    pp_msg = pp_msg; // touch
}

void msg_mon_recv_msg_process_created(Mon_Msg_Type *pp_msg) {
    pp_msg = pp_msg; // touch
}

void msg_mon_recv_msg_process_death(Mon_Msg_Type *pp_msg) {
    const char *WHERE = "msg_mon_recv_msg_process_death";
    Ms_Od_Type *lp_od;
    int         lv_max;
    int         lv_oid;

    lv_max = static_cast<int>(gv_ms_od_mgr.get_cap());
    for (lv_oid = 1; lv_oid < lv_max; lv_oid++) {
        lp_od = gv_ms_od_mgr.get_entry(lv_oid);
        if ((lp_od != NULL) && (lp_od->ip_stream != NULL)) {
#ifdef SQ_PHANDLE_VERIFIER
            if ((pp_msg->u.request.u.death.nid == lp_od->iv_nid) &&
                (pp_msg->u.request.u.death.pid == lp_od->iv_pid) &&
                (pp_msg->u.request.u.death.verifier == lp_od->iv_verif)) {
#else
            if ((pp_msg->u.request.u.death.nid == lp_od->iv_nid) &&
                (pp_msg->u.request.u.death.pid == lp_od->iv_pid)) {
#endif
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE, "open match, closing stream for %s\n",
                                       lp_od->ip_stream->get_name());
                SB_Trans::Trans_Stream::close_stream(WHERE,
                                                     lp_od->ip_stream,
                                                     pp_msg->u.request.u.death.aborted,
                                                     true,
                                                     false);
                lp_od->ip_stream = NULL;
            }
        }
    }
}

void msg_mon_recv_msg_shutdown(Mon_Msg_Type *pp_msg) {
    pp_msg = pp_msg; // touch
}

void msg_mon_recv_msg_unknown(Mon_Msg_Type *pp_msg) {
    pp_msg = pp_msg; // touch
}

void msg_mon_recv_notice_msg_loc_cbt(Mon_Msg_Type *pp_msg, int pv_size) {
    const char *WHERE = "msg_mon_recv_notice_msg_loc_cbt";
    MS_Md_Type *lp_md;

    lp_md = msg_mon_loc_get_md(pp_msg, pv_size);
    if (gv_ms_enable_messages)
        msg_mon_recv_msg_cbt(lp_md);
    else
        msg_mon_recv_msg_cbt_discard(WHERE, lp_md, false);
}

//
// Purpose: get registry
//
SB_Export int msg_mon_reg_get(MS_Mon_ConfigType    pv_config_type,
                              int                  pv_next,
                              char                *pp_group,
                              char                *pp_key,
                              MS_Mon_Reg_Get_Type *pp_info) {
    const char   *WHERE = "msg_mon_reg_get";
    Mon_Msg_Type *lp_msg;
    int           lv_inx;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_REG_GET);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REG_GET, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER type=%d, next=%d, group=%s, key=%s, info=%p\n",
                           pv_config_type, pv_next, pp_group, pp_key, pfp(pp_info));
    if (!gv_ms_mon_calls_ok) // msg_mon_reg_get
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    // send the get to the monitor
    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Get;
    lp_msg->u.request.u.get.nid = gv_ms_su_nid;
    lp_msg->u.request.u.get.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.get.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.get.process_name,
                        sizeof(lp_msg->u.request.u.get.process_name));
#endif
    lp_msg->u.request.u.get.type = static_cast<ConfigType>(pv_config_type);
    lp_msg->u.request.u.get.next = pv_next ? true : false;
    if (pp_group == NULL)
        lp_msg->u.request.u.get.group[0] = '\0';
    else
        ms_util_string_copy(lp_msg->u.request.u.get.group,
                            sizeof(lp_msg->u.request.u.get.group),
                            pp_group);
    ms_util_string_copy(lp_msg->u.request.u.get.key,
                        sizeof(lp_msg->u.request.u.get.key),
                        pp_key);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send get req to mon, p-id=%d/%d, type=%d, next=%d, group=%s, key=%s\n",
                           gv_ms_su_nid, gv_ms_su_pid,
                           pv_config_type, pv_next, pp_group, pp_key);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "get",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "get req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Get)) {
        if (gv_ms_trace_mon) {
            trace_where_printf(WHERE, "EXIT OK get req, type=%d, group=%s, num-keys=%d, num-returned=%d\n",
                               lp_msg->u.reply.u.get.type,
                               lp_msg->u.reply.u.get.group,
                               lp_msg->u.reply.u.get.num_keys,
                               lp_msg->u.reply.u.get.num_returned);
            for (lv_inx = 0;
                 lv_inx < lp_msg->u.reply.u.get.num_returned;
                 lv_inx++) {
                trace_where_printf(WHERE, "get req, list[%d] .key=%s, .value=%s\n",
                                   lv_inx,
                                   lp_msg->u.reply.u.get.list[lv_inx].key,
                                   lp_msg->u.reply.u.get.list[lv_inx].value);
            }
        }
        // copy results for user
        memcpy(pp_info, &lp_msg->u.reply.u.get, sizeof(MS_Mon_Reg_Get_Type));
    }

    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: set registry
//
SB_Export int msg_mon_reg_set(MS_Mon_ConfigType   pv_config_type,
                              char               *pp_group,
                              char               *pp_key,
                              char               *pp_value) {
    const char   *WHERE = "msg_mon_reg_set";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_REG_SET);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REG_SET, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER type=%d, group=%s, key=%s, value=%s\n",
                           pv_config_type, pp_group, pp_key, pp_value);
    if (!gv_ms_mon_calls_ok) // msg_mon_reg_set
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    // send the set to the monitor
    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Set;
    lp_msg->u.request.u.set.nid = gv_ms_su_nid;
    lp_msg->u.request.u.set.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.set.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.set.process_name,
                         sizeof(lp_msg->u.request.u.set.process_name));
#endif
    lp_msg->u.request.u.set.type = static_cast<ConfigType>(pv_config_type);
    if (pp_group == NULL)
        lp_msg->u.request.u.set.group[0] = '\0';
    else
        ms_util_string_copy(lp_msg->u.request.u.set.group,
                            sizeof(lp_msg->u.request.u.set.group),
                            pp_group);
    ms_util_string_copy(lp_msg->u.request.u.set.key,
                        sizeof(lp_msg->u.request.u.set.key),
                        pp_key);
    ms_util_string_copy(lp_msg->u.request.u.set.value,
                        sizeof(lp_msg->u.request.u.set.value),
                        pp_value);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send set req to mon, p-id=%d/%d, type=%d, group=%s, key=%s, value=%s\n",
                           gv_ms_su_nid, gv_ms_su_pid,
                           pv_config_type, pp_group, pp_key, pp_value);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "set",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "set req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK set req\n");
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE set, ret=%d\n",
                                   lv_mpierr);
        }
    }

    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: register for death notifications
//
SB_Export int msg_mon_register_death_notification(int pv_target_nid,
                                                  int pv_target_pid) {
    const char *WHERE = "msg_mon_register_death_notification";
    SB_API_CTR (lv_zctr, MSG_MON_REGISTER_DEATH_NOTIFICATION);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REG_DEATH_NOTIFICATION, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER target-p-id=%d/%d\n",
                           pv_target_nid, pv_target_pid);
    if (!gv_ms_mon_calls_ok) // msg_mon_register_death_notification
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    MS_Mon_Transid_Type lv_transid;
    TRANSID_SET_NULL(lv_transid);
    int lv_fserr = msg_mon_send_notify(WHERE, gv_ms_su_nid, gv_ms_su_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       false,
                                       pv_target_nid, pv_target_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       lv_transid, true);
    return lv_fserr;
}

//
// Purpose: register for death notifications
//
SB_Export int msg_mon_register_death_notification2(int pv_notify_nid,
                                                   int pv_notify_pid,
                                                   int pv_target_nid,
                                                   int pv_target_pid) {
    const char *WHERE = "msg_mon_register_death_notification2";
    SB_API_CTR (lv_zctr, MSG_MON_REGISTER_DEATH_NOTIFICATION2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REG_DEATH_NOTIFICATION2, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER notify-p-id=%d/%d, target-p-id=%d/%d\n",
                           pv_notify_nid, pv_notify_pid,
                           pv_target_nid, pv_target_pid);
    if (!gv_ms_mon_calls_ok) // msg_mon_register_death_notification2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    MS_Mon_Transid_Type lv_transid;
    TRANSID_SET_NULL(lv_transid);
    int lv_fserr = msg_mon_send_notify(WHERE, pv_notify_nid, pv_notify_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       false,
                                       pv_target_nid, pv_target_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       lv_transid, true);
    return lv_fserr;
}

//
// Purpose: register for death notifications (no assert errors)
//
SB_Export int msg_mon_register_death_notification3(int pv_target_nid,
                                                   int pv_target_pid) {
    const char *WHERE = "msg_mon_register_death_notification3";
    SB_API_CTR (lv_zctr, MSG_MON_REGISTER_DEATH_NOTIFICATION3);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REG_DEATH_NOTIFICATION3, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER target-p-id=%d/%d\n",
                           pv_target_nid, pv_target_pid);
    if (!gv_ms_mon_calls_ok) // msg_mon_register_death_notification
        return ms_err_rtn_msg_noassert(WHERE, "msg_init() or startup not called or shutdown",
                                        XZFIL_ERR_INVALIDSTATE);

    MS_Mon_Transid_Type lv_transid;
    TRANSID_SET_NULL(lv_transid);
    int lv_fserr = msg_mon_send_notify(WHERE, gv_ms_su_nid, gv_ms_su_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       false,
                                       pv_target_nid, pv_target_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       lv_transid, false);
    return lv_fserr;
}

#ifdef SQ_PHANDLE_VERIFIER
//
// Purpose: register for death notifications (no assert errors)
//
SB_Export int msg_mon_register_death_notification4(int           pv_target_nid,
                                                   int           pv_target_pid,
                                                   SB_Verif_Type pv_target_verif) {
    const char *WHERE = "msg_mon_register_death_notification4";
    SB_API_CTR (lv_zctr, MSG_MON_REGISTER_DEATH_NOTIFICATION4);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REG_DEATH_NOTIFICATION4, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER target-p-id=%d/%d/" PFVY "\n",
                           pv_target_nid, pv_target_pid, pv_target_verif);
    if (!gv_ms_mon_calls_ok) // msg_mon_register_death_notification
        return ms_err_rtn_msg_noassert(WHERE, "msg_init() or startup not called or shutdown",
                                        XZFIL_ERR_INVALIDSTATE);

    MS_Mon_Transid_Type lv_transid;
    TRANSID_SET_NULL(lv_transid);
    int lv_fserr = msg_mon_send_notify(WHERE, gv_ms_su_nid, gv_ms_su_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       false,
                                       pv_target_nid, pv_target_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       pv_target_verif,
#endif
                                       lv_transid,
                                       false);
    return lv_fserr;
}

//
// Purpose: register for death notifications
//
SB_Export int msg_mon_register_death_notification_name(const char *pp_target_name) {
    const char *WHERE = "msg_mon_register_death_notification_name";
    SB_API_CTR (lv_zctr, MSG_MON_REGISTER_DEATH_NOTIFICATION_NAME);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REG_DEATH_NOTIFICATION_NAME, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER target-name=%s\n", pp_target_name);
    if (!gv_ms_mon_calls_ok) // msg_mon_register_death_notification
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_target_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid target name (null)",
                              XZFIL_ERR_BOUNDSERR);

    MS_Mon_Transid_Type lv_transid;
    TRANSID_SET_NULL(lv_transid);
    int lv_fserr = msg_mon_send_notify_verif(WHERE, ga_ms_su_pname_seq,
                                             false,
                                             pp_target_name,
                                             lv_transid,
                                             true);
    return lv_fserr;
}

//
// Purpose: register for death notifications
//
SB_Export int msg_mon_register_death_notification_name2(const char *pp_notify_name,
                                                        const char *pp_target_name) {
    const char *WHERE = "msg_mon_register_death_notification_name2";
    SB_API_CTR (lv_zctr, MSG_MON_REGISTER_DEATH_NOTIFICATION_NAME2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REG_DEATH_NOTIFICATION_NAME2, 0);
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER notify-name=%s, target-name=%s\n",
                           pp_notify_name, pp_target_name);
    if (!gv_ms_mon_calls_ok) // msg_mon_register_death_notification2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_target_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid target name (null)",
                              XZFIL_ERR_BOUNDSERR);

    MS_Mon_Transid_Type lv_transid;
    TRANSID_SET_NULL(lv_transid);
    int lv_fserr = msg_mon_send_notify_verif(WHERE, pp_notify_name,
                                             false,
                                             pp_target_name,
                                             lv_transid,
                                             true);
    return lv_fserr;
}
#endif

//
// Purpose: re-open process [close/open]
//
SB_Export int msg_mon_reopen_process(SB_Phandle_Type *pp_phandle) {
    const char *WHERE = "msg_mon_reopen_process";
    SB_API_CTR (lv_zctr, MSG_MON_REOPEN_PROCESS);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_REOPEN_PROCESS, 0);
    if (gv_ms_trace_mon) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, pp_phandle);
        trace_where_printf(WHERE, "ENTER phandle=%s\n", la_phandle);
    }
    if (!gv_ms_calls_ok) // msg_mon_reopen_process
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    msg_mon_close_process_com(pp_phandle, false); // don't free oid
    Ms_Od_Type *lp_od = ms_od_map_phandle_to_od(pp_phandle);
    if (lp_od == NULL)
        return ms_err_rtn_msg(WHERE, "invalid phandle", XZFIL_ERR_BOUNDSERR);
    return static_cast<short>(
      msg_mon_open_process_com(lp_od->ia_process_name,  // name
                               pp_phandle,              // phandle
                               NULL,                    // oid
                               true,                    // reopen
                               lp_od->iv_death_notif,   // death notification
                               false,                   // self
                               false,                   // backup
                               false,                   // ic
                               false));                 // fs
}

//
// Purpose: send node-info
//
int msg_mon_send_node_info(const char   *pp_where,
                           Mon_Msg_Type *pp_msg,
                           int           pv_msg_err,
                           int           pv_nid,
                           bool          pv_cont) {
    const char  *lp_req;
    int          lv_mpierr;

    pp_msg->type = MsgType_Service;
    pp_msg->noreply = false;
    if (pv_cont) {
        // get context before it gets overwritten
        int lv_last_nid = pp_msg->u.reply.u.node_info.last_nid;
        int lv_last_pnid = pp_msg->u.reply.u.node_info.last_pnid;

        pp_msg->u.request.type = ReqType_NodeInfo;
        pp_msg->u.request.u.node_info.nid = gv_ms_su_nid;
        pp_msg->u.request.u.node_info.pid = gv_ms_su_pid;
        pp_msg->u.request.u.node_info.target_nid = pv_nid;
        pp_msg->u.request.u.node_info.continuation = true;
        // get context
        pp_msg->u.request.u.node_info.last_nid = lv_last_nid;
        pp_msg->u.request.u.node_info.last_pnid = lv_last_pnid;
        lp_req = "node-info-cont";
        if (gv_ms_trace_mon) {
            trace_where_printf(pp_where, "send %s req to mon, p-id=%d/%d, t-nid=%d\n",
                               lp_req, gv_ms_su_nid, gv_ms_su_pid, pv_nid);
            trace_where_printf(pp_where, "send %s req to mon, context (last_nid=%d, last_pnid=%d)\n",
                               lp_req,
                               pp_msg->u.request.u.node_info.last_nid,
                               pp_msg->u.request.u.node_info.last_pnid);
        }
    } else {
        pp_msg->u.request.type = ReqType_NodeInfo;
        pp_msg->u.request.u.node_info.nid = gv_ms_su_nid;
        pp_msg->u.request.u.node_info.pid = gv_ms_su_pid;
        pp_msg->u.request.u.node_info.target_nid = pv_nid;
        pp_msg->u.request.u.node_info.last_nid = -1;
        pp_msg->u.request.u.node_info.last_pnid = -1;
        pp_msg->u.request.u.node_info.continuation = false;
        lp_req = "node-info";
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "send %s req to mon, p-id=%d/%d, t-nid=%d\n",
                               lp_req, gv_ms_su_nid, gv_ms_su_pid, pv_nid);
    }
    lv_mpierr = msg_mon_sendrecv_mon(pp_where,
                                     lp_req,
                                     pp_msg,
                                     pv_msg_err);
    if (msg_mon_msg_ok(pp_where,
                       lp_req,
                       &lv_mpierr,
                       pp_msg,
                       MsgType_Service,
                       ReplyType_NodeInfo)) {
        struct NodeInfo_reply_def *lp_info = &pp_msg->u.reply.u.node_info;
        lv_mpierr = lp_info->return_code;
        int lv_cluster_nodes = lp_info->num_nodes;
        int lv_nodes = lp_info->num_returned;
        if (gv_ms_trace_mon) {
            if (lv_mpierr == MPI_SUCCESS) {
                trace_where_printf(pp_where, "%s req, cluster-nodes=%d, returned-nodes=%d, physical-nodes=%d, spare-nodes=%d, available-spare-nodes=%d\n",
                                   lp_req, lv_cluster_nodes, lv_nodes, lp_info->num_pnodes, lp_info->num_spares, lp_info->num_available_spares);
                for (int lv_node = 0; lv_node < lv_nodes; lv_node++) {
                    trace_where_printf(pp_where, "%s req[%d], nid=%d, state=%d, type=%d, cpus=%d, procs=%d\n",
                                       lp_req,
                                       lv_node,
                                       lp_info->node[lv_node].nid,
                                       lp_info->node[lv_node].state,
                                       lp_info->node[lv_node].type,
                                       lp_info->node[lv_node].processors,
                                       lp_info->node[lv_node].process_count);
                    trace_where_printf(pp_where, "%s req[%d], pnid=%d, pstate=%d, spare=%d, name=%s, cores=%d\n",
                                       lp_req,
                                       lv_node,
                                       lp_info->node[lv_node].pnid,
                                       lp_info->node[lv_node].pstate,
                                       lp_info->node[lv_node].spare_node,
                                       lp_info->node[lv_node].node_name,
                                       lp_info->node[lv_node].cores);
                }
            } else
                trace_where_printf(pp_where, "FAILED %s req, ret=%d\n",
                                   lp_req, lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(pp_where, "node-info EXIT", lv_mpierr);
}

//
// Purpose: send process-info
//
int msg_mon_send_process_info(const char    *pp_where,
                              Mon_Msg_Type  *pp_msg,
                              int            pv_msg_err,
                              int            pv_nid,
                              int            pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                              SB_Verif_Type  pv_verif,
#endif
                              char          *pp_name,
                              int            pv_ptype,
                              bool           pv_cont) {
    const char  *lp_req;
    int          lv_mpierr;

    pp_msg->type = MsgType_Service;
    pp_msg->noreply = false;
    if (pv_cont) {
        struct ProcessInfoCont_def lv_context;
        // get context before it gets overwritten
        for (int lv_inx = 0, inxctx = (MAX_PROCINFO_LIST - 1);
             lv_inx < MAX_PROC_CONTEXT;
             lv_inx++, inxctx--) {
            lv_context.context[lv_inx].nid =
              pp_msg->u.reply.u.process_info.process[inxctx].nid;
            lv_context.context[lv_inx].pid =
              pp_msg->u.reply.u.process_info.process[inxctx].pid;
        }
        pp_msg->u.request.type = ReqType_ProcessInfoCont;
        pp_msg->u.request.u.process_info_cont.nid = gv_ms_su_nid;
        pp_msg->u.request.u.process_info_cont.pid = gv_ms_su_pid;
        pp_msg->u.request.u.process_info_cont.type = static_cast<PROCESSTYPE>(pv_ptype);
        pp_msg->u.request.u.process_info_cont.allNodes =
          ((pv_nid == -1) && (pv_pid == -1));
        // get context
        memcpy(pp_msg->u.request.u.process_info_cont.context,
               lv_context.context,
               sizeof(lv_context.context));
        lp_req = "process-info-cont";
        if (gv_ms_trace_mon) {
            trace_where_printf(pp_where, "send %s req to mon, t-p-id=%d/%d, type=%d\n",
                               lp_req, pv_nid, pv_pid, pv_ptype);
            for (int lv_inx = 0; lv_inx < MAX_PROC_CONTEXT; lv_inx++)
                trace_where_printf(pp_where, "send %s req to mon, context[%d].p-id=%d/%d\n",
                                   lp_req,
                                   lv_inx,
                                   pp_msg->u.request.u.process_info_cont.context[lv_inx].nid,
                                   pp_msg->u.request.u.process_info_cont.context[lv_inx].pid);
        }
    } else {
        pp_msg->u.request.type = ReqType_ProcessInfo;
        pp_msg->u.request.u.process_info.nid = gv_ms_su_nid;
        pp_msg->u.request.u.process_info.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
        pp_msg->u.request.u.process_info.verifier = gv_ms_su_verif;
        ms_util_string_clear(pp_msg->u.request.u.process_info.process_name,
                             sizeof(pp_msg->u.request.u.process_info.process_name));
#endif
        pp_msg->u.request.u.process_info.target_nid = pv_nid;
        pp_msg->u.request.u.process_info.target_pid = pv_pid;
#ifdef SQ_PHANDLE_VERIFIER
        pp_msg->u.request.u.process_info.target_verifier = pv_verif;
#endif
        pp_msg->u.request.u.process_info.type = static_cast<PROCESSTYPE>(pv_ptype);
        ms_util_string_clear(pp_msg->u.request.u.process_info.target_process_pattern,
                             sizeof(pp_msg->u.request.u.process_info.target_process_pattern));
        if (pp_name == NULL) {
            ms_util_string_clear(pp_msg->u.request.u.process_info.target_process_name,
                                 sizeof(pp_msg->u.request.u.process_info.target_process_name));
        } else {
#ifdef SQ_PHANDLE_VERIFIER
            pp_msg->u.request.u.process_info.target_verifier = pv_verif;
            ms_util_name_seq(pp_name,
                             pp_msg->u.request.u.process_info.target_process_name,
                             sizeof(pp_msg->u.request.u.process_info.target_process_name),
                             &pp_msg->u.request.u.process_info.target_verifier);
#else
            ms_util_string_copy(pp_msg->u.request.u.process_info.target_process_name,
                                sizeof(pp_msg->u.request.u.process_info.target_process_name),
                                pp_name);
#endif
        }
        lp_req = "process-info";
#ifdef SQ_PHANDLE_VERIFIER
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "send %s req to mon, p-id=%d/%d/" PFVY ", t-p-id=%d/%d/" PFVY ", pname=%s, type=%d\n",
                               lp_req, gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_verif,
                               pv_nid, pv_pid, pv_verif,
                               pp_msg->u.request.u.process_info.target_process_name, pv_ptype);
#else
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "send %s req to mon, p-id=%d/%d, t-p-id=%d/%d, pname=%s, type=%d\n",
                               lp_req, gv_ms_su_nid, gv_ms_su_pid, pv_nid, pv_pid, pp_name, pv_ptype);
#endif
    }
    lv_mpierr = msg_mon_sendrecv_mon(pp_where,
                                     lp_req,
                                     pp_msg,
                                     pv_msg_err);
    if (msg_mon_msg_ok(pp_where,
                       lp_req,
                       &lv_mpierr,
                       pp_msg,
                       MsgType_Service,
                       ReplyType_ProcessInfo)) {
        struct ProcessInfo_reply_def *lp_info = &pp_msg->u.reply.u.process_info;
        lv_mpierr = lp_info->return_code;
        int lv_procs = lp_info->num_processes;
        if (gv_ms_trace_mon) {
            if (lv_mpierr == MPI_SUCCESS) {
                trace_where_printf(pp_where, "EXIT OK %s req, procs=%d, more-data=%d\n",
                                   lp_req, lv_procs, lp_info->more_data);
                if (lp_info->more_data) {
                    // only print if there's more data
                    for (int lv_inx = 0, inxctx = (MAX_PROCINFO_LIST - 1);
                         lv_inx < MAX_PROC_CONTEXT;
                         lv_inx++, inxctx--) {
                        trace_where_printf(pp_where, "EXIT OK %s req, context[%d].p-id=%d/%d\n",
                                           lp_req,
                                           lv_inx,
                                           lp_info->process[inxctx].nid,
                                           lp_info->process[inxctx].pid);
                    }
                }
                for (int lv_proc = 0; lv_proc < lv_procs; lv_proc++) {
#ifdef SQ_PHANDLE_VERIFIER
                    trace_where_printf(pp_where, "EXIT OK process-info rep[%d], p-id=%d/%d/" PFVY ", pname=%s, os_pid=%d, pri=%d\n",
                                       lv_proc,
                                       lp_info->process[lv_proc].nid,
                                       lp_info->process[lv_proc].pid,
                                       lp_info->process[lv_proc].verifier,
                                       lp_info->process[lv_proc].process_name,
                                       lp_info->process[lv_proc].os_pid,
                                       lp_info->process[lv_proc].priority);
#else
                    trace_where_printf(pp_where, "EXIT OK process-info rep[%d], p-id=%d/%d, pname=%s, os_pid=%d, pri=%d\n",
                                       lv_proc,
                                       lp_info->process[lv_proc].nid,
                                       lp_info->process[lv_proc].pid,
                                       lp_info->process[lv_proc].process_name,
                                       lp_info->process[lv_proc].os_pid,
                                       lp_info->process[lv_proc].priority);
#endif
#ifdef SQ_PHANDLE_VERIFIER
                    trace_where_printf(pp_where, "EXIT OK process-info rep[%d], p-p-id=%d/%d/" PFVY ", p-pname=%s, state=%d\n",
                                       lv_proc,
                                       lp_info->process[lv_proc].parent_nid,
                                       lp_info->process[lv_proc].parent_pid,
                                       lp_info->process[lv_proc].parent_verifier,
                                       lp_info->process[lv_proc].parent_name,
                                       lp_info->process[lv_proc].state);
#else
                    trace_where_printf(pp_where, "EXIT OK process-info rep[%d], p-p-id=%d/%d, p-pname=%s, state=%d\n",
                                       lv_proc,
                                       lp_info->process[lv_proc].parent_nid,
                                       lp_info->process[lv_proc].parent_pid,
                                       lp_info->process[lv_proc].parent_name,
                                       lp_info->process[lv_proc].state);
#endif
                    trace_where_printf(pp_where, "EXIT OK process-info rep[%d], eventmsg=%d, sysmsg=%d, paired=%d, penddel=%d, pendrep=%d\n",
                                       lv_proc,
                                       lp_info->process[lv_proc].event_messages,
                                       lp_info->process[lv_proc].system_messages,
                                       lp_info->process[lv_proc].paired,
                                       lp_info->process[lv_proc].pending_delete,
                                       lp_info->process[lv_proc].pending_replication);
                    trace_where_printf(pp_where, "EXIT OK process-info rep[%d], wstartup=%d, opened=%d, backup=%d\n",
                                       lv_proc,
                                       lp_info->process[lv_proc].waiting_startup,
                                       lp_info->process[lv_proc].opened,
                                       lp_info->process[lv_proc].backup);
                }
            } else
                trace_where_printf(pp_where, "EXIT FAILED %s req, ret=%d\n",
                                   lp_req, lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(pp_where, "EXIT", lv_mpierr);
}

int msg_mon_send_notify(const char          *pp_where,
                        int                  pv_notify_nid,
                        int                  pv_notify_pid,
#ifdef SQ_PHANDLE_VERIFIER
                        SB_Verif_Type        pv_notify_verif,
#endif
                        bool                 pv_cancel,
                        int                  pv_target_nid,
                        int                  pv_target_pid,
#ifdef SQ_PHANDLE_VERIFIER
                        SB_Verif_Type        pv_target_verif,
#endif
                        MS_Mon_Transid_Type  pv_target_transid,
                        bool                 pv_assert) {
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;

    if (gv_ms_trace_mon) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_target_transid);
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(pp_where, "ENTER n-p-id=%d/%d/" PFVY ", cancel=%d, t-p-id=%d/%d/" PFVY ", t-transid=%s\n",
                           pv_notify_nid, pv_notify_pid, pv_notify_verif, pv_cancel,
                           pv_target_nid, pv_target_pid, pv_target_verif, la_transid);
#else
        trace_where_printf(pp_where, "ENTER n-p-id=%d/%d, cancel=%d, t-p-id=%d/%d, t-transid=%s\n",
                           pv_notify_nid, pv_notify_pid, pv_cancel,
                           pv_target_nid, pv_target_pid, la_transid);
#endif
    }

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Notify;
    lp_msg->u.request.u.notify.nid = pv_notify_nid;
    lp_msg->u.request.u.notify.pid = pv_notify_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.notify.verifier = pv_notify_verif;
    ms_util_string_clear(lp_msg->u.request.u.notify.process_name,
                         sizeof(lp_msg->u.request.u.notify.process_name));
#endif
    lp_msg->u.request.u.notify.cancel = pv_cancel; // notice
    lp_msg->u.request.u.notify.target_nid = pv_target_nid;
    lp_msg->u.request.u.notify.target_pid = pv_target_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.notify.target_verifier = pv_target_verif;
    lp_msg->u.request.u.notify.fill1 = 0;
    ms_util_string_clear(lp_msg->u.request.u.notify.target_process_name,
                         sizeof(lp_msg->u.request.u.notify.target_process_name));
#else
    lp_msg->u.request.u.notify.fill1 = 0;
#endif
    TRANSID_COPY_MON_TO(lp_msg->u.request.u.notify.trans_id, pv_target_transid);

    if (gv_ms_trace_mon)
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(pp_where, "send notify req to mon, p-id=%d/%d/" PFVY "\n",
                           lp_msg->u.request.u.notify.nid,
                           lp_msg->u.request.u.notify.pid,
                           lp_msg->u.request.u.notify.verifier);
#else
        trace_where_printf(pp_where, "send notify req to mon, p-id=%d/%d\n",
                           lp_msg->u.request.u.notify.nid,
                           lp_msg->u.request.u.notify.pid);
#endif
    lv_mpierr = msg_mon_sendrecv_mon(pp_where,
                                     "notify",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(pp_where,
                       "notify req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(pp_where, "EXIT OK notify req, p-id=%d/%d, pname=%s\n",
                                   lp_msg->u.reply.u.generic.nid,
                                   lp_msg->u.reply.u.generic.pid,
                                   lp_msg->u.reply.u.generic.process_name);
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(pp_where, "EXIT FAILURE notify, ret=%d\n",
                                   lv_mpierr);
        }
    }
    if (pv_assert)
        return ms_err_mpi_rtn_msg(pp_where, "EXIT", lv_mpierr);
    else
        return ms_err_mpi_rtn_msg_noassert(pp_where, "EXIT", lv_mpierr);
}

#ifdef SQ_PHANDLE_VERIFIER
int msg_mon_send_notify_verif(const char          *pp_where,
                              const char          *pp_notify_name,
                              bool                 pv_cancel,
                              const char          *pp_target_name,
                              MS_Mon_Transid_Type  pv_target_transid,
                              bool                 pv_assert) {
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;

    if (gv_ms_trace_mon) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_target_transid);
        trace_where_printf(pp_where, "ENTER n-name=%s, cancel=%d, t-name=%s, t-transid=%s\n",
                           pp_notify_name, pv_cancel,
                           pp_target_name, la_transid);
    }

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Notify;
    lp_msg->u.request.u.notify.nid = -2;
    lp_msg->u.request.u.notify.pid = -2;
    ms_util_name_seq(pp_notify_name,
                     lp_msg->u.request.u.notify.process_name,
                     sizeof(lp_msg->u.request.u.notify.process_name),
                     &lp_msg->u.request.u.notify.verifier);
    lp_msg->u.request.u.notify.cancel = pv_cancel; // notice
    lp_msg->u.request.u.notify.target_nid = -2;
    lp_msg->u.request.u.notify.target_pid = -2;
    lp_msg->u.request.u.notify.fill1 = 0;
    ms_util_name_seq(pp_target_name,
                     lp_msg->u.request.u.notify.target_process_name,
                     sizeof(lp_msg->u.request.u.notify.target_process_name),
                     &lp_msg->u.request.u.notify.target_verifier);
    TRANSID_COPY_MON_TO(lp_msg->u.request.u.notify.trans_id, pv_target_transid);

    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "send notify req to mon, n-name=%s:" PFVY ", t-name=%s:" PFVY "\n",
                           lp_msg->u.request.u.notify.process_name,
                           lp_msg->u.request.u.notify.verifier,
                           lp_msg->u.request.u.notify.target_process_name,
                           lp_msg->u.request.u.notify.target_verifier);
    lv_mpierr = msg_mon_sendrecv_mon(pp_where,
                                     "notify",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(pp_where,
                       "notify req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(pp_where, "EXIT OK notify req, p-id=%d/%d, pname=%s\n",
                                   lp_msg->u.reply.u.generic.nid,
                                   lp_msg->u.reply.u.generic.pid,
                                   lp_msg->u.reply.u.generic.process_name);
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(pp_where, "EXIT FAILURE notify, ret=%d\n",
                                   lv_mpierr);
        }
    }
    if (pv_assert)
        return ms_err_mpi_rtn_msg(pp_where, "EXIT", lv_mpierr);
    else
        return ms_err_mpi_rtn_msg_noassert(pp_where, "EXIT", lv_mpierr);
}
#endif

//
// Purpose: send zone-info
//
int msg_mon_send_zone_info(const char   *pp_where,
                           Mon_Msg_Type *pp_msg,
                           int           pv_msg_err,
                           int           pv_nid,
                           int           pv_zid,
                           bool          pv_cont) {
    const char  *lp_req;
    int          lv_mpierr;

    pp_msg->type = MsgType_Service;
    pp_msg->noreply = false;
    if (pv_cont) {
        // get context before it gets overwritten
        int lv_last_nid = pp_msg->u.reply.u.zone_info.last_nid;
        int lv_last_pnid = pp_msg->u.reply.u.zone_info.last_pnid;

        pp_msg->u.request.type = ReqType_ZoneInfo;
        pp_msg->u.request.u.zone_info.nid = gv_ms_su_nid;
        pp_msg->u.request.u.zone_info.pid = gv_ms_su_pid;
        pp_msg->u.request.u.zone_info.target_nid = pv_nid;
        pp_msg->u.request.u.zone_info.target_zid = pv_zid;
        pp_msg->u.request.u.zone_info.continuation = true;
        // get context
        pp_msg->u.request.u.zone_info.last_nid = lv_last_nid;
        pp_msg->u.request.u.zone_info.last_pnid = lv_last_pnid;
        lp_req = "zone-info-cont";
        if (gv_ms_trace_mon) {
            trace_where_printf(pp_where, "send %s req to mon, p-id=%d/%d, t-nid=%d, t-zid=%d\n",
                               lp_req, gv_ms_su_nid, gv_ms_su_pid, pv_nid, pv_zid);
            trace_where_printf(pp_where, "send %s req to mon, context (last_nid=%d, last_pnid=%d), continuation= %d\n",
                               lp_req,
                               pp_msg->u.request.u.zone_info.last_nid,
                               pp_msg->u.request.u.zone_info.last_pnid,
                               pp_msg->u.request.u.zone_info.continuation);
        }
    } else {
        pp_msg->u.request.type = ReqType_ZoneInfo;
        pp_msg->u.request.u.zone_info.nid = gv_ms_su_nid;
        pp_msg->u.request.u.zone_info.pid = gv_ms_su_pid;
        pp_msg->u.request.u.zone_info.target_nid = pv_nid;
        pp_msg->u.request.u.zone_info.target_zid = pv_zid;
        pp_msg->u.request.u.zone_info.last_nid = -1;
        pp_msg->u.request.u.zone_info.last_pnid = -1;
        pp_msg->u.request.u.zone_info.continuation = false;
        lp_req = "node-info";
        if (gv_ms_trace_mon)
            trace_where_printf(pp_where, "send %s req to mon, p-id=%d/%d, t-nid=%d, t-zid=%d, continuation=%d\n",
                               lp_req,
                               gv_ms_su_nid,
                               gv_ms_su_pid,
                               pv_nid,
                               pv_zid,
                               pp_msg->u.request.u.zone_info.continuation);
    }
    lv_mpierr = msg_mon_sendrecv_mon(pp_where,
                                     lp_req,
                                     pp_msg,
                                     pv_msg_err);
    if (msg_mon_msg_ok(pp_where,
                       lp_req,
                       &lv_mpierr,
                       pp_msg,
                       MsgType_Service,
                       ReplyType_ZoneInfo)) {
        struct ZoneInfo_reply_def *lp_info = &pp_msg->u.reply.u.zone_info;
        lv_mpierr = lp_info->return_code;
        int lv_cluster_nodes = lp_info->num_nodes;
        int lv_nodes = lp_info->num_returned;
        if (gv_ms_trace_mon) {
            if (lv_mpierr == MPI_SUCCESS) {
                trace_where_printf(pp_where, "%s req, cluster-nodes=%d, returned-nodes=%d\n",
                                   lp_req, lv_cluster_nodes, lv_nodes);
                for (int lv_node = 0; lv_node < lv_nodes; lv_node++) {
                    trace_where_printf(pp_where, "%s req[%d], nid=%d, zid=%d, state=%d, node name=%s\n",
                                       lp_req,
                                       lv_node,
                                       lp_info->node[lv_node].nid,
                                       lp_info->node[lv_node].zid,
                                       lp_info->node[lv_node].pstate,
                                       lp_info->node[lv_node].node_name);
                }
            } else
                trace_where_printf(pp_where, "FAILED %s req, ret=%d\n",
                                   lp_req, lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(pp_where, "node-info EXIT", lv_mpierr);
}

//
// Purpose: sendrecv a message to monitor
//
int msg_mon_sendrecv_mon(const char   *pp_where,
                         const char   *pp_desc,
                         Mon_Msg_Type *pp_msg,
                         int           pv_msg_err) {
    int lv_mpierr;

    if (pv_msg_err)
        lv_mpierr = pv_msg_err;
    else
        lv_mpierr = msg_mon_sendrecv_mon_lio(pp_where, pp_desc, pp_msg);
    return lv_mpierr;
}

//
// Purpose: sendrecv a message to monitor via local IO
//
int msg_mon_sendrecv_mon_lio(const char   *pp_where,
                             const char   *pp_desc,
                             Mon_Msg_Type *pp_msg) {
    int lv_mpierr;
    int lv_recv_tag;
    int lv_slot;

    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "send-recv %s request to mon\n", pp_desc);
    // acquire semaphore, allocate recv tag, and put in mon msg
    // semaphore is used to limit sendrecv's to limit of slot manager
    lv_slot = gv_ms_recv_tag_mgr.alloc_if_cap();
    if (lv_slot >= 0) {
        lv_recv_tag = lv_slot + SB_Trans::MS_TAG_MON_MIN;
        pp_msg->reply_tag = lv_recv_tag;

        if (gp_local_mon_io->send_recv(pp_msg)) {
            lv_mpierr = ms_err_errno_to_mpierr(pp_where);
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_LOCIO_SEND_RECV,
                               errno,
                               lv_mpierr);
        } else
            lv_mpierr = MPI_SUCCESS;

        // free recv tag and release semaphore
        gv_ms_recv_tag_mgr.free_slot(lv_slot);
    } else
        lv_mpierr = MPI_ERR_NO_MEM;

    if (gv_ms_trace_mon)
        trace_where_printf(pp_where, "received %s reply from mon, ret=%d\n",
                           pp_desc, lv_mpierr);
    return lv_mpierr; // return mpi error
}

//
// Purpose: shutdown (system level)
//
SB_Export int msg_mon_shutdown(int pv_level) {
    const char   *WHERE = "msg_mon_shutdown";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;

#ifdef USE_SB_API_CTRS
    gv_sb_api_ctr_mgr.report();
#endif
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER level=%d\n", pv_level);

    if (!gv_ms_mon_calls_ok) // msg_mon_shutdown
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Shutdown;
    lp_msg->u.request.u.shutdown.nid = gv_ms_su_nid;
    lp_msg->u.request.u.shutdown.pid = gv_ms_su_pid;
    lp_msg->u.request.u.shutdown.level = static_cast<ShutdownLevel>(pv_level);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send shutdown req to mon, p-id=%d/%d, level=%d\n",
                           lp_msg->u.request.u.shutdown.nid,
                           lp_msg->u.request.u.shutdown.pid,
                           pv_level);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "shutdown",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "shutdown req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK shutdown req\n");
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE shutdown, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}



//
// Purpose: start a process
//
SB_Export int msg_mon_start_process(char             *pp_prog,
                                    char             *pp_name,
                                    char             *pp_ret_name,
                                    int               pv_argc,
                                    char            **ppp_argv,
                                    SB_Phandle_Type  *pp_phandle,
                                    int               pv_open,
                                    int              *pp_oid,
                                    int               pv_ptype,
                                    int               pv_priority,
                                    int               pv_debug,
                                    int               pv_backup,
                                    int              *pp_nid,
                                    int              *pp_pid,
                                    char             *pp_infile,
                                    char             *pp_outfile
#ifdef SQ_PHANDLE_VERIFIER
                                   ,SB_Verif_Type    *pp_verif
#endif
                                   ) {
    const char *WHERE = "msg_mon_start_process";
    char       *lp_ldpath;
    char       *lp_path;
    int         lv_fserr;
    SB_API_CTR (lv_zctr, MSG_MON_START_PROCESS);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_START_PROCESS, 0);
    if (!gv_ms_mon_calls_ok) // msg_mon_start_process
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    lp_path = getenv("PATH");
    lp_ldpath = getenv("LD_LIBRARY_PATH");
    lv_fserr = msg_mon_start_process_ph1(NULL,         // cb
                                         false,        // nowait
                                         lp_path,
                                         lp_ldpath,
                                         pp_prog,
                                         pp_name,
                                         pp_ret_name,
                                         pv_argc,
                                         ppp_argv,
                                         pp_phandle,
                                         pv_ptype,
                                         pv_priority,
                                         pv_debug,
                                         pv_backup,
                                         0,         // tag
                                         pp_nid,
                                         pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         pp_verif,
#endif
                                         pp_infile,
                                         pp_outfile,
                                         false); // unhooked
    if ((lv_fserr == XZFIL_ERR_OK) && pv_open)
        lv_fserr = msg_mon_open_process(pp_name, pp_phandle, pp_oid);
    return lv_fserr;
}

//
// Purpose: start a process
//
SB_Export int msg_mon_start_process2(char             *pp_prog,
                                     char             *pp_name,
                                     char             *pp_ret_name,
                                     int               pv_argc,
                                     char            **ppp_argv,
                                     SB_Phandle_Type  *pp_phandle,
                                     int               pv_open,
                                     int              *pp_oid,
                                     int               pv_ptype,
                                     int               pv_priority,
                                     int               pv_debug,
                                     int               pv_backup,
                                     int              *pp_nid,
                                     int              *pp_pid,
                                     char             *pp_infile,
                                     char             *pp_outfile,
                                     int               pv_unhooked
#ifdef SQ_PHANDLE_VERIFIER
                                    ,SB_Verif_Type    *pp_verif
#endif
                                    ) {
    const char *WHERE = "msg_mon_start_process2";
    char       *lp_ldpath;
    char       *lp_path;
    int         lv_fserr;
    SB_API_CTR (lv_zctr, MSG_MON_START_PROCESS2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_START_PROCESS2, 0);
    if (!gv_ms_mon_calls_ok) // msg_mon_start_process2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    lp_path = getenv("PATH");
    lp_ldpath = getenv("LD_LIBRARY_PATH");
    lv_fserr = msg_mon_start_process_ph1(NULL,         // cb
                                         false,        // nowait
                                         lp_path,
                                         lp_ldpath,
                                         pp_prog,
                                         pp_name,
                                         pp_ret_name,
                                         pv_argc,
                                         ppp_argv,
                                         pp_phandle,
                                         pv_ptype,
                                         pv_priority,
                                         pv_debug,
                                         pv_backup,
                                         0,         // tag
                                         pp_nid,
                                         pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         pp_verif,
#endif
                                         pp_infile,
                                         pp_outfile,
                                         pv_unhooked);
    if ((lv_fserr == XZFIL_ERR_OK) && pv_open)
        lv_fserr = msg_mon_open_process(pp_name, pp_phandle, pp_oid);
    return lv_fserr;
}

//
// Purpose: start a process ph1
//
int msg_mon_start_process_ph1(MS_Mon_Start_Process_Cb_Type  pv_callback,
                              bool                          pv_nowait,
                              char                         *pp_path,
                              char                         *pp_ldpath,
                              char                         *pp_prog,
                              char                         *pp_name,
                              char                         *pp_ret_name,
                              int                           pv_argc,
                              char                        **ppp_argv,
                              SB_Phandle_Type              *pp_phandle,
                              int                           pv_ptype,
                              int                           pv_priority,
                              int                           pv_debug,
                              int                           pv_backup,
                              long long                     pv_tag,
                              int                          *pp_nid,
                              int                          *pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                              SB_Verif_Type                *pp_verif,
#endif
                              char                         *pp_infile,
                              char                         *pp_outfile,
                              int                           pv_unhooked) {
    const char   *WHERE = "msg_mon_start_process_ph1";
    Mon_Msg_Type *lp_msg;
    int           lv_argc;
    int           lv_mpierr;

#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER cb=%p, nowait=%d, path=%s, ldpath=%s, prog=%s, pname=%s, retname=%p, phandle=%p, type=%d, pri=%d, debug=%d, bu=%d, tag=0x%llx, nid=%d, pid=%p, verifier=%p, infile=%s, outfile=%s, unhooked=%d\n",
                           SB_CB_TO_PTR(pv_callback), pv_nowait, pp_path, pp_ldpath, pp_prog,
                           pp_name, pp_ret_name,
                           pfp(pp_phandle), pv_ptype,
                           pv_priority, pv_debug, pv_backup, pv_tag,
                           (pp_nid == NULL) ? -1 : *pp_nid,
                           pfp(pp_pid), pfp(pp_verif),
                           pp_infile, pp_outfile, pv_unhooked);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER cb=%p, nowait=%d, path=%s, ldpath=%s, prog=%s, pname=%s, retname=%p, phandle=%p, type=%d, pri=%d, debug=%d, bu=%d, tag=0x%llx, nid=%d, infile=%s, outfile=%s, unhooked=%d\n",
                           SB_CB_TO_PTR(pv_callback), pv_nowait, pp_path, pp_ldpath, pp_prog,
                           pp_name, pp_ret_name,
                           pfp(pp_phandle), pv_ptype,
                           pv_priority, pv_debug, pv_backup, pv_tag,
                           (pp_nid == NULL) ? -1 : *pp_nid,
                           pp_infile, pp_outfile, pv_unhooked);
#endif
    if (pv_nowait || pv_callback) {
        Ms_TC_Node *lp_node = static_cast<Ms_TC_Node *>(gv_ms_tc_map.get(pv_tag));
        if (lp_node != NULL) {
            int lv_fserr = XZFIL_ERR_BADPARMVALUE;
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE new_process, ret=%d\n",
                                   lv_fserr);
            return lv_fserr;
        }
    }
    if (pv_callback) {
        Ms_TC_Node *lp_node = new_TC_Node(pv_tag, pv_callback);
        gv_ms_tc_map.put(&lp_node->iv_link);
    }

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_NewProcess;
    lp_msg->u.request.u.new_process.nid = (pp_nid == NULL) ? -1 : *pp_nid;
    lp_msg->u.request.u.new_process.type = static_cast<PROCESSTYPE>(pv_ptype);
    lp_msg->u.request.u.new_process.priority = pv_priority;
    lp_msg->u.request.u.new_process.debug = pv_debug;
    lp_msg->u.request.u.new_process.backup = pv_backup;
    lp_msg->u.request.u.new_process.unhooked = pv_unhooked;
    lp_msg->u.request.u.new_process.nowait = pv_nowait;
    lp_msg->u.request.u.new_process.tag = pv_tag;
    ms_util_string_copy(lp_msg->u.request.u.new_process.process_name,
                        sizeof(lp_msg->u.request.u.new_process.process_name),
                        pp_name);
    ms_util_string_copy(lp_msg->u.request.u.new_process.path,
                        sizeof(lp_msg->u.request.u.new_process.path),
                        pp_path);
    ms_util_string_copy(lp_msg->u.request.u.new_process.ldpath,
                        sizeof(lp_msg->u.request.u.new_process.ldpath),
                        pp_ldpath);
    ms_util_string_copy(lp_msg->u.request.u.new_process.program,
                        sizeof(lp_msg->u.request.u.new_process.program),
                        pp_prog);
    if (pv_argc >= 1)
        lv_argc = pv_argc - 1; // don't do arg[0]
    else
        lv_argc = 0;
    lp_msg->u.request.u.new_process.argc = lv_argc;
    for (int lv_arg = 1; lv_arg <= lv_argc; lv_arg++) {
        ms_util_string_copy(lp_msg->u.request.u.new_process.argv[lv_arg-1],
                            sizeof(lp_msg->u.request.u.new_process.argv[lv_arg-1]),
                            ppp_argv[lv_arg]);
        lp_msg->u.request.u.new_process.argv[lv_arg-1][MAX_ARG_SIZE-1] = '\0';
    }
    if (pp_infile != NULL)
        ms_util_string_copy(lp_msg->u.request.u.new_process.infile,
                            sizeof(lp_msg->u.request.u.new_process.infile),
                            pp_infile);
    else
        ms_util_string_clear(lp_msg->u.request.u.new_process.infile,
                             sizeof(lp_msg->u.request.u.new_process.infile));
    if (pp_outfile != NULL)
        ms_util_string_copy(lp_msg->u.request.u.new_process.outfile,
                            sizeof(lp_msg->u.request.u.new_process.outfile),
                            pp_outfile);
    else
        ms_util_string_clear(lp_msg->u.request.u.new_process.outfile,
                             sizeof(lp_msg->u.request.u.new_process.outfile));
    lp_msg->u.request.u.new_process.fill1 = 0;

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send new_process req to mon, nid=%d, pname=%s, path=%s\n",
                           lp_msg->u.request.u.new_process.nid,
                           pp_name, pp_path);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "new_process",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "new_process req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_NewProcess)) {
        lv_mpierr = lp_msg->u.reply.u.new_process.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (pp_phandle != NULL)
                ms_util_fill_phandle_name(pp_phandle,
                                          lp_msg->u.reply.u.new_process.process_name,
                                          lp_msg->u.reply.u.new_process.nid,
                                          lp_msg->u.reply.u.new_process.pid
#ifdef SQ_PHANDLE_VERIFIER
                                         ,lp_msg->u.reply.u.new_process.verifier
#endif
                                         );
            if (gv_ms_trace_mon) {
                char la_phandle[MSG_UTIL_PHANDLE_LEN];
                if (pp_phandle != NULL)
                    msg_util_format_phandle(la_phandle, pp_phandle);
                else
                    strcpy(la_phandle, "(nil)");
#ifdef SQ_PHANDLE_VERIFIER
                trace_where_printf(WHERE, "EXIT OK new_process req, p-id=%d/%d/" PFVY ", pname=%s, phandle=%s\n",
                                   lp_msg->u.reply.u.new_process.nid,
                                   lp_msg->u.reply.u.new_process.pid,
                                   lp_msg->u.reply.u.new_process.verifier,
                                   lp_msg->u.reply.u.new_process.process_name,
                                   la_phandle);
#else
                trace_where_printf(WHERE, "EXIT OK new_process req, p-id=%d/%d, pname=%s, phandle=%s\n",
                                   lp_msg->u.reply.u.new_process.nid,
                                   lp_msg->u.reply.u.new_process.pid,
                                   lp_msg->u.reply.u.new_process.process_name,
                                   la_phandle);
#endif
            }
            if (pp_ret_name != NULL)
                strcpy(pp_ret_name, lp_msg->u.reply.u.new_process.process_name);
            if (pp_nid != NULL)
                *pp_nid = lp_msg->u.reply.u.new_process.nid;
            if (pp_pid != NULL)
                *pp_pid = lp_msg->u.reply.u.new_process.pid;
#ifdef SQ_PHANDLE_VERIFIER
            if (pp_verif != NULL)
                *pp_verif = lp_msg->u.reply.u.new_process.verifier;
#endif
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE new_process, ret=%d\n",
                                   lv_mpierr);
        }
    }
    if (lv_mpierr != MPI_SUCCESS)
        gv_ms_tc_map.remove(pv_tag);
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: start a process (nowait)
//
SB_Export int msg_mon_start_process_nowait(char             *pp_prog,
                                           char             *pp_name,
                                           char             *pp_ret_name,
                                           int               pv_argc,
                                           char            **ppp_argv,
                                           SB_Phandle_Type  *pp_phandle,
                                           int               pv_ptype,
                                           int               pv_priority,
                                           int               pv_debug,
                                           int               pv_backup,
                                           long long         pv_tag,
                                           int              *pp_nid,
                                           int              *pp_pid,
                                           char             *pp_infile,
                                           char             *pp_outfile
#ifdef SQ_PHANDLE_VERIFIER
                                          ,SB_Verif_Type    *pp_verif
#endif
                                          ) {
    const char   *WHERE = "msg_mon_start_process_nowait";
    char         *lp_ldpath;
    char         *lp_path;
    int           lv_fserr;
    SB_API_CTR   (lv_zctr, MSG_MON_START_PROCESS_NOWAIT);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_START_PROCESS_NOWAIT, 0);

    if (!gv_ms_mon_calls_ok) // msg_mon_start_process_nowait
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    lp_path = getenv("PATH");
    lp_ldpath = getenv("LD_LIBRARY_PATH");
    lv_fserr = msg_mon_start_process_ph1(NULL,         // cb
                                         true,         // nowait
                                         lp_path,
                                         lp_ldpath,
                                         pp_prog,
                                         pp_name,
                                         pp_ret_name,
                                         pv_argc,
                                         ppp_argv,
                                         pp_phandle,
                                         pv_ptype,
                                         pv_priority,
                                         pv_debug,
                                         pv_backup,
                                         pv_tag,
                                         pp_nid,
                                         pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         pp_verif,
#endif
                                         pp_infile,
                                         pp_outfile,
                                         false); // unhooked
    return lv_fserr;
}

//
// Purpose: start a process (nowait)
//
SB_Export int msg_mon_start_process_nowait2(char             *pp_prog,
                                            char             *pp_name,
                                            char             *pp_ret_name,
                                            int               pv_argc,
                                            char            **ppp_argv,
                                            SB_Phandle_Type  *pp_phandle,
                                            int               pv_ptype,
                                            int               pv_priority,
                                            int               pv_debug,
                                            int               pv_backup,
                                            long long         pv_tag,
                                            int              *pp_nid,
                                            int              *pp_pid,
                                            char             *pp_infile,
                                            char             *pp_outfile,
                                            int               pv_unhooked
#ifdef SQ_PHANDLE_VERIFIER
                                           ,SB_Verif_Type    *pp_verif
#endif
                                           ) {
    const char   *WHERE = "msg_mon_start_process_nowait2";
    char         *lp_ldpath;
    char         *lp_path;
    int           lv_fserr;
    SB_API_CTR   (lv_zctr, MSG_MON_START_PROCESS_NOWAIT2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_START_PROCESS_NOWAIT2, 0);

    if (!gv_ms_mon_calls_ok) // msg_mon_start_process_nowait2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    lp_path = getenv("PATH");
    lp_ldpath = getenv("LD_LIBRARY_PATH");
    lv_fserr = msg_mon_start_process_ph1(NULL,         // cb
                                         true,         // nowait
                                         lp_path,
                                         lp_ldpath,
                                         pp_prog,
                                         pp_name,
                                         pp_ret_name,
                                         pv_argc,
                                         ppp_argv,
                                         pp_phandle,
                                         pv_ptype,
                                         pv_priority,
                                         pv_debug,
                                         pv_backup,
                                         pv_tag,
                                         pp_nid,
                                         pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         pp_verif,
#endif
                                         pp_infile,
                                         pp_outfile,
                                         pv_unhooked);
    return lv_fserr;
}

//
// Purpose: start a process (nowait-callback)
//
SB_Export int msg_mon_start_process_nowait_cb(MS_Mon_Start_Process_Cb_Type  pv_callback,
                                              char                         *pp_prog,
                                              char                         *pp_name,
                                              char                         *pp_ret_name,
                                              int                           pv_argc,
                                              char                        **ppp_argv,
                                              int                           pv_ptype,
                                              int                           pv_priority,
                                              int                           pv_debug,
                                              int                           pv_backup,
                                              long long                     pv_tag,
                                              int                          *pp_nid,
                                              int                          *pp_pid,
                                              char                         *pp_infile,
                                              char                         *pp_outfile
#ifdef SQ_PHANDLE_VERIFIER
                                             ,SB_Verif_Type                *pp_verif
#endif
                                             ) {
    const char   *WHERE = "msg_mon_start_process_nowait_cb";
    char         *lp_ldpath;
    char         *lp_path;
    int           lv_fserr;
    SB_API_CTR   (lv_zctr, MSG_MON_START_PROCESS_NOWAIT_CB);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_START_PROCESS_NOWAIT_CB, 0);

    if (!gv_ms_mon_calls_ok) // msg_mon_start_process_nowait_cb
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    lp_path = getenv("PATH");
    lp_ldpath = getenv("LD_LIBRARY_PATH");
    lv_fserr = msg_mon_start_process_ph1(pv_callback,
                                         true,         // nowait
                                         lp_path,
                                         lp_ldpath,
                                         pp_prog,
                                         pp_name,
                                         pp_ret_name,
                                         pv_argc,
                                         ppp_argv,
                                         NULL, // phandle
                                         pv_ptype,
                                         pv_priority,
                                         pv_debug,
                                         pv_backup,
                                         pv_tag,
                                         pp_nid,
                                         pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         pp_verif,
#endif
                                         pp_infile,
                                         pp_outfile,
                                         false); // unhooked
    return lv_fserr;
}

//
// Purpose: start a process (nowait-callback)
//
SB_Export int msg_mon_start_process_nowait_cb2(MS_Mon_Start_Process_Cb_Type  pv_callback,
                                               char                         *pp_prog,
                                               char                         *pp_name,
                                               char                         *pp_ret_name,
                                               int                           pv_argc,
                                               char                        **ppp_argv,
                                               int                           pv_ptype,
                                               int                           pv_priority,
                                               int                           pv_debug,
                                               int                           pv_backup,
                                               long long                     pv_tag,
                                               int                          *pp_nid,
                                               int                          *pp_pid,
                                               char                         *pp_infile,
                                               char                         *pp_outfile,
                                               int                           pv_unhooked
#ifdef SQ_PHANDLE_VERIFIER
                                              ,SB_Verif_Type                *pp_verif
#endif
                                              ) {
    const char   *WHERE = "msg_mon_start_process_nowait_cb2";
    char         *lp_ldpath;
    char         *lp_path;
    int           lv_fserr;
    SB_API_CTR   (lv_zctr, MSG_MON_START_PROCESS_NOWAIT_CB2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_START_PROCESS_NOWAIT_CB2, 0);

    if (!gv_ms_mon_calls_ok) // msg_mon_start_process_nowait_cb2
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    lp_path = getenv("PATH");
    lp_ldpath = getenv("LD_LIBRARY_PATH");
    lv_fserr = msg_mon_start_process_ph1(pv_callback,
                                         true,         // nowait
                                         lp_path,
                                         lp_ldpath,
                                         pp_prog,
                                         pp_name,
                                         pp_ret_name,
                                         pv_argc,
                                         ppp_argv,
                                         NULL, // phandle
                                         pv_ptype,
                                         pv_priority,
                                         pv_debug,
                                         pv_backup,
                                         pv_tag,
                                         pp_nid,
                                         pp_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                         pp_verif,
#endif
                                         pp_infile,
                                         pp_outfile,
                                         pv_unhooked);
    return lv_fserr;
}

#ifdef SQ_STFSD
//
// Purpose: issue stfsd message
//
SB_Export int msg_mon_stfsd_send(void  *pp_data,
                                 int    pv_input_len,
                                 void  *pp_rsp_data,
                                 int   *pp_output_len,
                                 int    pv_tag) {
    const char   *WHERE = "msg_mon_stfsd_send";
    Mon_Msg_Type *lp_msg;
    int           lv_len;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_STFSD_SEND);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_STFSD_SEND, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER data=%p, len=%d, tag=%d\n",
                           pp_data, pv_input_len, pv_tag);
    if (!gv_ms_mon_calls_ok) // msg_mon_stfsd_send
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Stfsd;
    lp_msg->u.request.u.stfsd.nid = gv_ms_su_nid;
    lp_msg->u.request.u.stfsd.pid = gv_ms_su_pid;
    lp_msg->u.request.u.stfsd.length = pv_input_len;
    lp_msg->u.request.u.stfsd.tag = pv_tag;

    memcpy(lp_msg->u.request.u.stfsd.data, pp_data, pv_input_len);

    if (gv_ms_trace_mon) {
        trace_where_printf(WHERE, "send stfsd req to mon, p-id=%d/%d\n",
                           lp_msg->u.request.u.stfsd.nid,
                           lp_msg->u.request.u.stfsd.pid);
        trace_print_data(pp_data, pv_input_len, gv_ms_trace_data_max);
    }

    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "stfsd",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "stfsd req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Stfsd)) {
        lv_mpierr = lp_msg->u.reply.u.stfsd.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            *pp_output_len = lp_msg->u.reply.u.stfsd.length;
            memcpy(pp_rsp_data,
                   lp_msg->u.reply.u.stfsd.data,
                   *pp_output_len);
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK stfsd, reply length=%d\n",
                                   *pp_output_len);
        } else {
            *pp_output_len = 0;
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE stfsd, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}
#endif

//
// Purpose: stop a process
//
SB_Export int msg_mon_stop_process(char *pp_name, int pv_nid, int pv_pid) {
    const char   *WHERE = "msg_mon_stop_process";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_STOP_PROCESS);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_STOP_PROCESS, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=%s, p-id=%d/%d\n",
                           pp_name, pv_nid, pv_pid);
    if (!gv_ms_mon_calls_ok) // msg_mon_stop_process
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Kill;
    lp_msg->u.request.u.kill.nid = gv_ms_su_nid;
    lp_msg->u.request.u.kill.pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_msg->u.request.u.kill.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.kill.process_name,
                         sizeof(lp_msg->u.request.u.kill.process_name));
#endif
    lp_msg->u.request.u.kill.target_nid = pv_nid;
    lp_msg->u.request.u.kill.target_pid = pv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    ms_util_name_seq(pp_name,
                     lp_msg->u.request.u.kill.target_process_name,
                     sizeof(lp_msg->u.request.u.kill.target_process_name),
                     &lp_msg->u.request.u.kill.target_verifier);
#else
    ms_util_string_copy(lp_msg->u.request.u.kill.target_process_name,
                        sizeof(lp_msg->u.request.u.kill.target_process_name),
                        pp_name);
#endif
    lp_msg->u.request.u.kill.persistent_abort = false;

#ifdef SQ_PHANDLE_VERIFIER
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send kill req to mon, p-id=%d/%d/" PFVY ", t-p-id=%d/%d/" PFVY ", t-name=%s:" PFVY "\n",
                           gv_ms_su_nid,
                           gv_ms_su_pid,
                           gv_ms_su_verif,
                           pv_nid,
                           pv_pid,
                           lp_msg->u.request.u.kill.verifier,
                           lp_msg->u.request.u.kill.target_process_name,
                           lp_msg->u.request.u.kill.target_verifier);
#else
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send kill req to mon, p-id=%d/%d, t-p-id=%d/%d, t-name=%s\n",
                           lp_msg->u.request.u.kill.nid,
                           lp_msg->u.request.u.kill.pid,
                           pv_nid, pv_pid, pp_name);
#endif
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "kill",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "kill req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK kill req\n");
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE kill, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

#ifdef SQ_PHANDLE_VERIFIER
SB_Export int msg_mon_stop_process_name(const char *pp_name) {
    const char   *WHERE = "msg_mon_stop_process_name";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_STOP_PROCESS2);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_STOP_PROCESS2, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER pname=%s\n", pp_name);
    if (!gv_ms_mon_calls_ok) // msg_mon_stop_process
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_name == NULL)
        return ms_err_rtn_msg(WHERE, "invalid name (null)",
                              XZFIL_ERR_BOUNDSERR);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_Kill;
    lp_msg->u.request.u.kill.nid = gv_ms_su_nid;
    lp_msg->u.request.u.kill.pid = gv_ms_su_pid;
    lp_msg->u.request.u.kill.verifier = gv_ms_su_verif;
    ms_util_string_clear(lp_msg->u.request.u.kill.process_name,
                         sizeof(lp_msg->u.request.u.kill.process_name));
    lp_msg->u.request.u.kill.target_nid = -1;
    lp_msg->u.request.u.kill.target_pid = -1;
    ms_util_name_seq(pp_name,
                     lp_msg->u.request.u.kill.target_process_name,
                     sizeof(lp_msg->u.request.u.kill.target_process_name),
                     &lp_msg->u.request.u.kill.target_verifier);
    lp_msg->u.request.u.kill.persistent_abort = false;

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send kill req to mon, p-id=%d/%d/" PFVY ", t-name=%s:" PFVY "\n",
                           gv_ms_su_nid,
                           gv_ms_su_pid,
                           gv_ms_su_verif,
                           lp_msg->u.request.u.kill.target_process_name,
                           lp_msg->u.request.u.kill.target_verifier);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "kill",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "kill req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (lv_mpierr == MPI_SUCCESS) {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT OK kill req\n");
        } else {
            if (gv_ms_trace_mon)
                trace_where_printf(WHERE, "EXIT FAILURE kill, ret=%d\n",
                                   lv_mpierr);
        }
    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}
#endif
//
// Purpose: return text for device state
//
const char *msg_mon_text_get_mon_device_state(MS_MON_DEVICE_STATE pv_state) {
    return SB_get_label_ms_mon_device_state(pv_state);
}

//
// Purpose: return text for config type
//
const char *msg_mon_text_get_mon_config_type(MS_Mon_ConfigType pv_ct) {
    return SB_get_label_ms_mon_configtype(pv_ct);
}

//
// Purpose: return text for msg type
//
const char *msg_mon_text_get_mon_msg_type(MS_Mon_MSGTYPE pv_mt) {
    return SB_get_label_ms_mon_msgtype(pv_mt);
}

//
// Purpose: return text for proc state
//
const char *msg_mon_text_get_mon_proc_state(MS_MON_PROC_STATE pv_state) {
    return SB_get_label_ms_mon_proc_state(pv_state);
}

//
// Purpose: return text for process type
//
const char *msg_mon_text_get_mon_process_type(MS_Mon_PROCESSTYPE pv_pt) {
    return SB_get_label_ms_mon_processtype(pv_pt);
}

//
// Purpose: return text for req state
//
const char *msg_mon_text_get_mon_req_type(MS_Mon_REQTYPE pv_rt) {
    return SB_get_label_ms_mon_reqtype(pv_rt);
}

//
// Purpose: return text for shutdown level
//
const char *msg_mon_text_get_mon_shutdown_level(MS_MON_ShutdownLevel pv_sl) {
    return SB_get_label_ms_mon_shutdownlevel(pv_sl);
}

//
// Purpose: return text for zone type
//
const char *msg_mon_text_get_mon_zone_type(MS_MON_ZoneType pv_zt) {
    const char *lp_ret;

    if (pv_zt == MS_Mon_ZoneType_Undefined)
        lp_ret = "MS_Mon_ZoneType_Undefined";
    else if (pv_zt == MS_Mon_ZoneType_Any)
        lp_ret = "MS_Mon_ZoneType_Any";
    else if (pv_zt == MS_Mon_ZoneType_Frontend)
        lp_ret = "MS_Mon_ZoneType_Frontend";
    else if (pv_zt == MS_Mon_ZoneType_Backend)
        lp_ret = "MS_Mon_ZoneType_Backend";
    else
        lp_ret = "MS_Mon_ZoneType_<unknown>";

    return lp_ret;
}

//
// Purpose: set TM leader
//
SB_Export int msg_mon_tm_leader_set(int *pp_nid, int *pp_pid, char *pp_name) {
    const char   *WHERE = "msg_mon_tm_leader_set";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_TM_LEADER_SET);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_TM_LEADER_SET, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER nid=%p, pid=%p, pname=%p\n",
                           pfp(pp_nid), pfp(pp_pid), pp_name);
    if (!gv_ms_mon_calls_ok) // msg_mon_tm_leader_set
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_TmLeader;
    lp_msg->u.request.u.leader.nid = gv_ms_su_nid;
    lp_msg->u.request.u.leader.pid = gv_ms_su_pid;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send set tm leader req to mon, p-id=%d/%d\n",
                           gv_ms_su_nid, gv_ms_su_pid);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "set tm leader",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "set tm leader req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (pp_nid != NULL)
            *pp_nid = lp_msg->u.reply.u.generic.nid;
        if (pp_pid != NULL)
            *pp_pid = lp_msg->u.reply.u.generic.pid;
        if (pp_name != NULL)
            strcpy(pp_name, lp_msg->u.reply.u.generic.process_name);
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXIT %s set tm leader req, ret=%d, p-id=%d/%d, pname=%s\n",
                               (lv_mpierr == MPI_SUCCESS) ? "OK" : "FAILURE",
                               lv_mpierr,
                               lp_msg->u.reply.u.generic.nid,
                               lp_msg->u.reply.u.generic.pid,
                               lp_msg->u.reply.u.generic.process_name);

    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: set TM ready for transactions
//
SB_Export int msg_mon_tm_ready(void) {
    const char   *WHERE = "msg_mon_tm_ready";
    Mon_Msg_Type *lp_msg;
    int           lv_mpierr;
    SB_API_CTR   (lv_zctr, MSG_MON_TM_READY);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_TM_READY, 0);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER\n");
    if (!gv_ms_mon_calls_ok) // msg_mon_tm_ready
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    Mon_Msg_Auto lv_msg;
    lp_msg = &lv_msg;
    lp_msg->type = MsgType_Service;
    lp_msg->noreply = false;
    lp_msg->u.request.type = ReqType_TmReady;
    lp_msg->u.request.u.tm_ready.nid = gv_ms_su_nid;
    lp_msg->u.request.u.tm_ready.pid = gv_ms_su_pid;
    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "send set tm ready req to mon, p-id=%d/%d\n",
                           gv_ms_su_nid, gv_ms_su_pid);
    lv_mpierr = msg_mon_sendrecv_mon(WHERE,
                                     "set tm ready",
                                     lp_msg,
                                     lv_msg.get_error());
    if (msg_mon_msg_ok(WHERE,
                       "set tm ready req",
                       &lv_mpierr,
                       lp_msg,
                       MsgType_Service,
                       ReplyType_Generic)) {
        lv_mpierr = lp_msg->u.reply.u.generic.return_code;
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "EXIT %s set tm ready req, ret=%d, p-id=%d/%d, pname=%s\n",
                               (lv_mpierr == MPI_SUCCESS) ? "OK" : "FAILURE",
                               lv_mpierr,
                               lp_msg->u.reply.u.generic.nid,
                               lp_msg->u.reply.u.generic.pid,
                               lp_msg->u.reply.u.generic.process_name);

    }
    return ms_err_mpi_rtn_msg(WHERE, "EXIT", lv_mpierr);
}

//
// Purpose: delist trans
//
SB_Export int msg_mon_trace_register_change(MS_Mon_Trace_Cb_Type pv_callback) {
    const char *WHERE = "msg_mon_trace_register_change";
    int         lv_fserr = XZFIL_ERR_OK;
    SB_API_CTR (lv_zctr, MSG_MON_TRACE_REGISTER_CHANGE);

    if (gv_ms_trace_mon)
        trace_where_printf(WHERE, "ENTER\n");
    if ((gv_ms_trace_callback_inx + 1) > MS_MAX_TRACE_CBS)
        lv_fserr = XZFIL_ERR_BOUNDSERR;
    else {
        ga_ms_trace_callback[gv_ms_trace_callback_inx] = pv_callback;
        gv_ms_trace_callback_inx++;
    }
    if (gv_ms_trace_mon) {
        if (lv_fserr == XZFIL_ERR_OK)
            trace_where_printf(WHERE, "EXIT OK\n");
        else
            trace_where_printf(WHERE, "EXIT FAILURE fserr=%d\n", lv_fserr);
    }
    return lv_fserr;
}

void msg_mon_trace_msg(const char *pp_where, Mon_Msg_Type *pp_msg) {

    switch (pp_msg->type) {
    case MsgType_Change:
        msg_mon_trace_msg_change(pp_where, pp_msg);
        break;
    case MsgType_Close:
        msg_mon_trace_msg_close(pp_where, pp_msg);
        break;
    case MsgType_NodeDown:
        msg_mon_trace_msg_node_down(pp_where, pp_msg);
        break;
    case MsgType_NodeQuiesce:
        msg_mon_trace_msg_node_quiesce(pp_where, pp_msg);
        break;
    case MsgType_NodeUp:
        msg_mon_trace_msg_node_up(pp_where, pp_msg);
        break;
    case MsgType_Open:
        msg_mon_trace_msg_open(pp_where, pp_msg);
        break;
    case MsgType_ProcessCreated:
        msg_mon_trace_msg_process_created(pp_where, pp_msg);
        break;
    case MsgType_ProcessDeath:
        msg_mon_trace_msg_process_death(pp_where, pp_msg);
        break;
    case MsgType_Shutdown:
        msg_mon_trace_msg_shutdown(pp_where, pp_msg);
        break;
    default:
        msg_mon_trace_msg_unknown(pp_where, pp_msg);
        break;
    }
}

void msg_mon_trace_msg_change(const char *pp_where, Mon_Msg_Type *pp_msg) {
    trace_where_printf(pp_where, "mon-msg-change type=%d, group=%s, key=%s, value=%s\n",
                       pp_msg->u.request.u.change.type,
                       pp_msg->u.request.u.change.group,
                       pp_msg->u.request.u.change.key,
                       pp_msg->u.request.u.change.value);
}

void msg_mon_trace_msg_close(const char *pp_where, Mon_Msg_Type *pp_msg) {
#ifdef SQ_PHANDLE_VERIFIER
    trace_where_printf(pp_where, "mon-msg-close p-id=%d/%d/" PFVY ", pname=%s, aborted=%d, mon=%d\n",
#else
    trace_where_printf(pp_where, "mon-msg-close p-id=%d/%d, pname=%s, aborted=%d, mon=%d\n",
#endif
                       pp_msg->u.request.u.close.nid,
                       pp_msg->u.request.u.close.pid,
#ifdef SQ_PHANDLE_VERIFIER
                       pp_msg->u.request.u.close.verifier,
#endif
                       pp_msg->u.request.u.close.process_name,
                       pp_msg->u.request.u.close.aborted,
                       pp_msg->u.request.u.close.mon);
}

void msg_mon_trace_msg_node_down(const char *pp_where, Mon_Msg_Type *pp_msg) {
    trace_where_printf(pp_where, "mon-msg-node-down nid=%d, node-name=%s\n",
                       pp_msg->u.request.u.down.nid,
                       pp_msg->u.request.u.down.node_name);
}

void msg_mon_trace_msg_node_quiesce(const char   *pp_where,
                                    Mon_Msg_Type *pp_msg) {
    trace_where_printf(pp_where, "mon-msg-node-quiesce nid=%d, node-name=%s\n",
                       pp_msg->u.request.u.quiesce.nid,
                       pp_msg->u.request.u.quiesce.node_name);
}

void msg_mon_trace_msg_node_up(const char *pp_where, Mon_Msg_Type *pp_msg) {
    trace_where_printf(pp_where, "mon-msg-node-up nid=%d, node-name=%s\n",
                       pp_msg->u.request.u.up.nid,
                       pp_msg->u.request.u.up.node_name);
}

void msg_mon_trace_msg_open(const char *pp_where, Mon_Msg_Type *pp_msg) {
#ifdef SQ_PHANDLE_VERIFIER
    trace_where_printf(pp_where, "mon-msg-open p-id=%d/%d/" PFVY ", pname=%s\n",
#else
    trace_where_printf(pp_where, "mon-msg-open p-id=%d/%d, pname=%s\n",
#endif
                       pp_msg->u.request.u.open.nid,
                       pp_msg->u.request.u.open.pid,
#ifdef SQ_PHANDLE_VERIFIER
                       pp_msg->u.request.u.open.verifier,
#endif
                       pp_msg->u.request.u.open.target_process_name);
}

void msg_mon_trace_msg_process_created(const char   *pp_where,
                                       Mon_Msg_Type *pp_msg) {
#ifdef SQ_PHANDLE_VERIFIER
    trace_where_printf(pp_where, "mon-msg-process-created p-id=%d/%d/" PFVY ", tag=0x%llx, port=%s, pname=%s, ret=%d\n",
#else
    trace_where_printf(pp_where, "mon-msg-process-created p-id=%d/%d, tag=0x%llx, port=%s, pname=%s, ret=%d\n",
#endif
                       pp_msg->u.request.u.process_created.nid,
                       pp_msg->u.request.u.process_created.pid,
#ifdef SQ_PHANDLE_VERIFIER
                       pp_msg->u.request.u.process_created.verifier,
#endif
                       pp_msg->u.request.u.process_created.tag,
                       pp_msg->u.request.u.process_created.port,
                       pp_msg->u.request.u.process_created.process_name,
                       pp_msg->u.request.u.process_created.return_code);
}

void msg_mon_trace_msg_process_death(const char   *pp_where,
                                     Mon_Msg_Type *pp_msg) {
    char la_transid[100];
    MS_Mon_Transid_Type lv_transid_copy;
    TRANSID_COPY_MON_FROM(lv_transid_copy, pp_msg->u.request.u.death.trans_id);
    msg_util_format_transid(la_transid, lv_transid_copy);
#ifdef SQ_PHANDLE_VERIFIER
    trace_where_printf(pp_where, "mon-msg-death p-id=%d/%d/" PFVY ", aborted=%d, transid=%s, pname=%s\n",
#else
    trace_where_printf(pp_where, "mon-msg-death p-id=%d/%d, aborted=%d, transid=%s, pname=%s\n",
#endif
                       pp_msg->u.request.u.death.nid,
                       pp_msg->u.request.u.death.pid,
#ifdef SQ_PHANDLE_VERIFIER
                       pp_msg->u.request.u.death.verifier,
#endif
                       pp_msg->u.request.u.death.aborted,
                       la_transid,
                       pp_msg->u.request.u.death.process_name);
}

void msg_mon_trace_msg_shutdown(const char *pp_where, Mon_Msg_Type *pp_msg) {
    trace_where_printf(pp_where, "mon-msg-shutdown p-id=%d/%d, level=%d\n",
                       pp_msg->u.request.u.shutdown.nid,
                       pp_msg->u.request.u.shutdown.pid,
                       pp_msg->u.request.u.shutdown.level);
}

void msg_mon_trace_msg_unknown(const char *pp_where, Mon_Msg_Type *pp_msg) {
    pp_msg = pp_msg; // touch
    trace_where_printf(pp_where, "mon-msg-unknown, type=%d\n", pp_msg->type);
}

//
// Purpose: delist trans
//
SB_Export int msg_mon_trans_delist(int                 pv_tm_nid,
                                   int                 pv_tm_pid,
                                   MS_Mon_Transid_Type pv_transid) {
    const char *WHERE = "msg_mon_trans_delist";
    SB_API_CTR (lv_zctr, MSG_MON_TRANS_DELIST);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_TRANS_DELIST, 0);
    if (gv_ms_trace_trans) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_transid);
        trace_where_printf(WHERE, "ENTER tm-p-id=%d/%d, transid=%s\n",
                           pv_tm_nid, pv_tm_pid, la_transid);
    }
    if (!gv_ms_mon_calls_ok) // msg_mon_trans_delist
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    int lv_fserr = msg_mon_send_notify(WHERE, pv_tm_nid, pv_tm_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       true,
                                       gv_ms_su_nid, gv_ms_su_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       pv_transid, true);
    return lv_fserr;
}

//
// Purpose: end trans
//
SB_Export int msg_mon_trans_end(int                 pv_tm_nid,
                                int                 pv_tm_pid,
                                MS_Mon_Transid_Type pv_transid) {
    const char *WHERE = "msg_mon_trans_end";
    SB_API_CTR (lv_zctr, MSG_MON_TRANS_END);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_TRANS_END, 0);
    if (gv_ms_trace_trans) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_transid);
        trace_where_printf(WHERE, "ENTER tm-p-id=%d/%d, transid=%s\n",
                           pv_tm_nid, pv_tm_pid, la_transid);
    }
    if (!gv_ms_mon_calls_ok) // msg_mon_trans_end
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    int lv_fserr = msg_mon_send_notify(WHERE, pv_tm_nid, pv_tm_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       true,
                                       -1, -1,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       pv_transid, true);
    return lv_fserr;
}

//
// Purpose: enlist trans
//
SB_Export int msg_mon_trans_enlist(int                 pv_tm_nid,
                                   int                 pv_tm_pid,
                                   MS_Mon_Transid_Type pv_transid) {
    const char *WHERE = "msg_mon_trans_enlist";
    SB_API_CTR (lv_zctr, MSG_MON_TRANS_ENLIST);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_MON_TRANS_ENLIST, 0);
    if (gv_ms_trace_trans) {
        char la_transid[100];
        msg_util_format_transid(la_transid, pv_transid);
        trace_where_printf(WHERE, "ENTER tm-p-id=%d/%d, transid=%s\n",
                           pv_tm_nid, pv_tm_pid, la_transid);
    }
    if (!gv_ms_mon_calls_ok) // msg_mon_trans_end
        return ms_err_rtn_msg(WHERE, "msg_init() or startup not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    int lv_fserr = msg_mon_send_notify(WHERE, pv_tm_nid, pv_tm_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       false,
                                       gv_ms_su_nid, gv_ms_su_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                       -1,
#endif
                                       pv_transid, true);
    return lv_fserr;
}

//
// Purpose: register tmlib
//
SB_Export int msg_mon_trans_register_tmlib(MS_Mon_Tmlib_Cb_Type pv_callback) {
    const char *WHERE = "msg_mon_trans_register_tmlib";
    SB_API_CTR (lv_zctr, MSG_MON_TRANS_REGISTER_TMLIB);

    if (gv_ms_trace_trans)
        trace_where_printf(WHERE, "ENTER\n");
    // don't check gv_ms_inited [may be called after static initializer]
    gv_ms_tmlib_callback = pv_callback;
    int lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_trans)
        trace_where_printf(WHERE, "EXIT OK, ret=%d\n", lv_fserr);
    return lv_fserr;
}

//
// Purpose: register tmlib2
//
SB_Export int msg_mon_trans_register_tmlib2(MS_Mon_Tmlib2_Cb_Type pv_callback) {
    const char *WHERE = "msg_mon_trans_register_tmlib2";
    SB_API_CTR (lv_zctr, MSG_MON_TRANS_REGISTER_TMLIB2);

    if (gv_ms_trace_trans)
        trace_where_printf(WHERE, "ENTER\n");
    // don't check gv_ms_inited [may be called after static initializer]
    gv_ms_tmlib2_callback = pv_callback;
    int lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_trans)
        trace_where_printf(WHERE, "EXIT OK, ret=%d\n", lv_fserr);
    return lv_fserr;
}

SB_Export void  msg_test_openers_close() {
    SB_Trans::Trans_Stream::close_nidpid_streams(false);
}

SB_Export void  msg_test_openers_del() {
    SB_Trans::Trans_Stream::delete_streams(true);
}

//
// Purpose: alloc od
//
Ms_Od_Type *ms_od_alloc() {
    const char *WHERE = "md_od_alloc";
    Ms_Od_Type *lp_od;
    size_t      lv_oid;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "OD-inuse-count=%d\n",
                           static_cast<int>(gv_ms_od_mgr.get_inuse()));
    lv_oid = gv_ms_od_mgr.alloc_entry();
    lp_od = gv_ms_od_mgr.get_entry(lv_oid);
    lp_od->iv_mutex.setname("mutex-od.iv_mutex");
    strcpy(lp_od->ia_prog, "?");
    lp_od->ip_stream = NULL;
    lp_od->iv_ref_count = 1;
    lp_od->iv_need_open = false;
    lp_od->iv_death_notif = true; // default
    lp_od->iv_fs_open = false; // default
    lp_od->iv_fs_closed = false; // default
    lp_od->iv_self = false; // default
    lp_od->iv_inuse = true;
    ms_od_cleanup_add(static_cast<int>(lv_oid));
    return lp_od;
}

void ms_od_cleanup_add(int pv_oid) {
    const char *WHERE = "ms_od_cleanup_add";
    SB_Ts_Imap *lp_map;
    SB_ML_Type *lp_entry;

    lp_map =
      static_cast<SB_Ts_Imap *>(SB_Thread::Sthr::specific_get(gv_ms_od_tls_inx));
    if (lp_map != NULL) {
        if (gv_ms_trace)
            trace_where_printf(WHERE, "add oid=%d\n", pv_oid);
        lp_entry = new SB_ML_Type;
        lp_entry->iv_id.i = pv_oid;
        lp_map->put(lp_entry);
    }
}

void ms_od_cleanup_key_dtor(void *pp_map) {
    const char   *WHERE = "ms_od_cleanup_key_dtor";
    SB_Imap_Enum *lp_enum;
    SB_Ts_Imap   *lp_map;
    int           lv_fserr;
    bool          lv_more;
    short         lv_oid;
    int           lv_status;

    lp_map = static_cast<SB_Ts_Imap *>(pp_map);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER map=%p\n", pp_map);
    if (lp_map != NULL) {
        // the TLS would be null if set is called after dtor is called
        lv_status = SB_Thread::Sthr::specific_set(gv_ms_od_tls_inx, pp_map);
        SB_util_assert_ieq(lv_status, 0);
        do {
            lp_enum = lp_map->keys();
            lv_more = lp_enum->more();
            if (lv_more) {
                lv_oid = static_cast<short>(lp_enum->next()->iv_id.i);
                if (gv_ms_trace)
                    trace_where_printf(WHERE, "oid=%d\n", lv_oid);
                lv_fserr = msg_mon_close_process_oid(lv_oid, true, false);
                if (lv_fserr != XZFIL_ERR_OK) {
                    // oid not there - clean it up
                    ms_od_cleanup_remove(lv_oid);
                }
            }
            delete lp_enum;
        } while (lv_more);
        delete lp_map;
        lv_status = SB_Thread::Sthr::specific_set(gv_ms_od_tls_inx, NULL);
        SB_util_assert_ieq(lv_status, 0);
    }
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT\n");
}

int ms_od_cleanup_enable() {
    SB_Ts_Imap *lp_map;
    int         lv_status;

    lp_map = new SB_Ts_Imap();
    lv_status = SB_Thread::Sthr::specific_set(gv_ms_od_tls_inx, lp_map);
    SB_util_assert_ieq(lv_status, 0);
    return XZFIL_ERR_OK;
}

void ms_od_cleanup_remove(int pv_oid) {
    const char *WHERE = "ms_od_cleanup_remove";
    SB_ML_Type *lp_entry;
    SB_Ts_Imap *lp_map;

    lp_map =
      static_cast<SB_Ts_Imap *>(SB_Thread::Sthr::specific_get(gv_ms_od_tls_inx));
    if (lp_map != NULL) {
        lp_entry =
          static_cast<SB_ML_Type *>(lp_map->remove(pv_oid));
        if (lp_entry != NULL) {
            if (gv_ms_trace)
                trace_where_printf(WHERE, "remove oid=%d\n", pv_oid);
            delete lp_entry;
        }
    }
}

//
// Purpose: shutdown fs
//
void ms_fs_shutdown_ph1() {
    void             *lp_handle;
    Fs_Shutdown_Type  lv_shutdown;

    lp_handle = dlopen("libsbfs.so", RTLD_NOW);
    if (lp_handle != NULL) {
        lv_shutdown.ipshutdown = dlsym(lp_handle, "fs_int_shutdown_ph1");
        if (lv_shutdown.ipshutdown != NULL)
            lv_shutdown.ishutdown();
    }
}

//
// Purpose: shutdown fs
//
void ms_fs_shutdown_ph2() {
    void             *lp_handle;
    Fs_Shutdown_Type  lv_shutdown;

    lp_handle = dlopen("libsbfs.so", RTLD_NOW);
    if (lp_handle != NULL) {
        lv_shutdown.ipshutdown = dlsym(lp_handle, "fs_int_shutdown_ph2");
        if (lv_shutdown.ipshutdown != NULL)
            lv_shutdown.ishutdown();
    }
}

//
// Purpose: free od
//
void ms_od_free(Ms_Od_Type *pp_od,
                bool        pv_free_oid,
                bool        pv_local,
                bool        pv_chkref) {
    const char *WHERE = "ms_od_free";

    SB_Trans::Trans_Stream *lp_stream = pp_od->ip_stream;
    if (lp_stream != NULL) {
        if (gv_ms_trace_mon)
            trace_where_printf(WHERE, "closing stream for %s\n",
                               lp_stream->get_name());
        SB_Trans::Trans_Stream::close_stream(WHERE,
                                             lp_stream,
                                             pv_local,
                                             true,
                                             false);
        pp_od->ip_stream = NULL;
    }
    if (pv_free_oid) {
        if ((!pv_chkref) || (pv_chkref && (pp_od->iv_ref_count <= 1))) {
            ms_od_cleanup_remove(pp_od->iv_oid);
            ms_od_map_name_remove(pp_od->ia_process_name);
            gv_ms_od_mgr.free_entry(pp_od->iv_oid);
        }
        if (gv_ms_trace)
            trace_where_printf(WHERE, "OD-inuse-count=%d\n",
                               static_cast<int>(gv_ms_od_mgr.get_inuse()));
    } else
        pp_od->ia_port[0] = '\0';
}

//
// Purpose: get my phandle
//
void ms_od_get_my_phandle(SB_Phandle_Type *pp_phandle) {
    memcpy(pp_phandle, &gv_ms_su_phandle, sizeof(SB_Phandle));
}

//
// Purpose: lock od
//
void *ms_od_lock(int pv_oid) {
    Ms_Od_Type *lp_od;
    int         lv_status;

    lp_od = ms_od_map_oid_to_od(pv_oid);
    SB_util_assert_pne(lp_od, NULL);
    lv_status = lp_od->iv_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    return lp_od;
}

//
// Purpose: add name to od map
//
void ms_od_map_name_add(char *pp_name, Ms_Od_Type *pp_od) {
    int lv_status;

    lv_status = gv_ms_od_name_map_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    gv_ms_od_name_map.removev(pp_name); // in case it's already there
    gv_ms_od_name_map.putv(pp_name, pp_od);
    lv_status = gv_ms_od_name_map_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: remove name from od map
//
void ms_od_map_name_remove(char *pp_name) {
    int lv_status;

    lv_status = gv_ms_od_name_map_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    gv_ms_od_name_map.removev(pp_name);
    lv_status = gv_ms_od_name_map_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: map name to od
//
Ms_Od_Type *ms_od_map_name_to_od(char *pp_name) {
    Ms_Od_Type *lp_od;
    int         lv_status;

    lv_status = gv_ms_od_name_map_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    lp_od = static_cast<Ms_Od_Type *>(gv_ms_od_name_map.getv(pp_name));
    lv_status = gv_ms_od_name_map_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);

    return lp_od;
}

//
// Purpose: map id to od
//
Ms_Od_Type *ms_od_map_oid_to_od(int pv_oid) {
    Ms_Od_Type *lp_od;
    int         lv_max;

    lv_max = static_cast<int>(gv_ms_od_mgr.get_cap());
    if ((pv_oid > 0) && (pv_oid < lv_max)) {
        lp_od = gv_ms_od_mgr.get_entry(pv_oid);
        if ((lp_od != NULL) && (!lp_od->iv_inuse))
            lp_od = NULL;
    } else
        lp_od = NULL;
    return lp_od;
}

//
// Purpose: map id to stream
//
SB_Trans::Stream_Base *ms_od_map_oid_to_stream(int pv_oid) {
    Ms_Od_Type            *lp_od = ms_od_map_oid_to_od(pv_oid);
    SB_Trans::Stream_Base *lp_stream;

    if (lp_od != NULL)
        lp_stream = lp_od->ip_stream;
    else
        lp_stream = NULL;
    return lp_stream;
}

//
// Purpose: map phandle to name
//
char *ms_od_map_phandle_to_name(SB_Phandle_Type *pp_phandle) {
    if (pp_phandle == NULL)
        return const_cast<char *>("<NULL>");
    SB_Phandle *lp_phandle = reinterpret_cast<SB_Phandle *>(pp_phandle);
    return reinterpret_cast<char *>(&lp_phandle->ia_name);
}

//
// Purpose: map phandle to oid
//
Ms_Od_Type *ms_od_map_phandle_to_od(SB_Phandle_Type *pp_phandle) {
    int lv_oid = ms_od_map_phandle_to_oid(pp_phandle);
    return ms_od_map_oid_to_od(lv_oid);
}

//
// Purpose: map phandle to oid
//
int ms_od_map_phandle_to_oid(SB_Phandle_Type *pp_phandle) {
    SB_Phandle *lp_phandle = reinterpret_cast<SB_Phandle *>(pp_phandle);
    return lp_phandle->iv_oid;
}

//
// Purpose: map phandle to stream
//
SB_Trans::Stream_Base *ms_od_map_phandle_to_stream(SB_Phandle_Type *pp_phandle) {
    Ms_Od_Type            *lp_od = ms_od_map_phandle_to_od(pp_phandle);
    SB_Trans::Stream_Base *lp_stream;

    if (lp_od != NULL)
        lp_stream = lp_od->ip_stream;
    else
        lp_stream = NULL;
    return lp_stream;
}

//
// Purpose: add reference to od
//
int ms_od_ref_add(Ms_Od_Type *pp_od) {
    pp_od->iv_ref_count++;
    return pp_od->iv_ref_count;
}

//
// Purpose: remove reference from od
//
int ms_od_ref_dec(Ms_Od_Type *pp_od) {
    pp_od->iv_ref_count--;
    return pp_od->iv_ref_count;
}

//
// Purpose: unlock od
//
void ms_od_unlock(void *pp_od) {
    Ms_Od_Type *lp_od;
    int         lv_status;

    lp_od = static_cast<Ms_Od_Type *>(pp_od);
    lv_status = lp_od->iv_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: Return true if valid oid
//
bool ms_od_valid_oid(int pv_oid) {
    return (ms_od_map_oid_to_od(pv_oid) != NULL);
}

//
// Purpose: acquire stats
//
void ms_stats(const char *pp_value, bool pv_trace) {
    char                  la_line[100];
    int                   lv_ms;
    static bool           lv_prev_ok = false;
    struct timeval        lv_t;
    static struct timeval lv_t_prev;
    struct tm             lv_tx;
    long long             lv_us_cpu;
    long long             lv_us_wc;
    struct rusage         lv_usage;
    static struct rusage  lv_usage_prev;

    gettimeofday(&lv_t, NULL);
    localtime_r(&lv_t.tv_sec, &lv_tx);
    getrusage(RUSAGE_SELF, &lv_usage);
    lv_ms = static_cast<int>(lv_t.tv_usec) / 1000;
    lv_us_cpu = static_cast<int>(lv_t.tv_usec) - lv_ms * 1000;
    sprintf(la_line, "time-of-day..................................=%02d:%02d:%02d.%03d.%03lld\n",
            lv_tx.tm_hour, lv_tx.tm_min, lv_tx.tm_sec, lv_ms, lv_us_cpu);
    ms_stats_print(pv_trace, la_line);
    if (lv_prev_ok) {
        lv_us_wc = (lv_t.tv_sec * SB_US_PER_SEC + lv_t.tv_usec) -
                   (lv_t_prev.tv_sec * SB_US_PER_SEC + lv_t_prev.tv_usec);
        sprintf(la_line, "elapsed us...................................=%lld\n",
                lv_us_wc);
        ms_stats_print(pv_trace, la_line);
    }
    lv_us_cpu = lv_usage.ru_utime.tv_sec * SB_US_PER_SEC +
                lv_usage.ru_utime.tv_usec +
                lv_usage.ru_stime.tv_sec * SB_US_PER_SEC +
                lv_usage.ru_stime.tv_usec;
    sprintf(la_line, "stat.utime...................................=%ld.%06ld\n",
            lv_usage.ru_utime.tv_sec,
            lv_usage.ru_utime.tv_usec);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.stime...................................=%ld.%06ld\n",
            lv_usage.ru_stime.tv_sec,
            lv_usage.ru_stime.tv_usec);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.maxrss (max resident set size)..........=%ld\n",
            lv_usage.ru_maxrss);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.ixrss (integral shared memory size).....=%ld\n",
            lv_usage.ru_ixrss);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.idrss (integral unshared data size).....=%ld\n",
            lv_usage.ru_idrss);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.isrss (integral unshared stack size)....=%ld\n",
            lv_usage.ru_ixrss);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.minflt (page reclaims)..................=%ld\n",
            lv_usage.ru_minflt);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.majflt (page faults)....................=%ld\n",
            lv_usage.ru_majflt);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.nswap (swaps)...........................=%ld\n",
            lv_usage.ru_nswap);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.inblock (block input operations)........=%ld\n",
            lv_usage.ru_inblock);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.oublock (block output operations).......=%ld\n",
            lv_usage.ru_oublock);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.msgsnd (messages sent)..................=%ld\n",
            lv_usage.ru_msgsnd);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.msgrcv (messages received)..............=%ld\n",
            lv_usage.ru_msgrcv);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.nsignals (signals received).............=%ld\n",
            lv_usage.ru_nsignals);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.nvcsw (voluntary context switches)......=%ld\n",
            lv_usage.ru_nvcsw);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "stat.nivcsw (involuntary context switches)...=%ld\n",
            lv_usage.ru_nivcsw);
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "voluntary context switches/sec (cpu).........=%.3f\n",
            static_cast<double>(lv_usage.ru_nvcsw) * SB_US_PER_SEC / static_cast<double>(lv_us_cpu));
    ms_stats_print(pv_trace, la_line);
    sprintf(la_line, "involuntary context switches/sec (cpu).......=%.3f\n",
            static_cast<double>(lv_usage.ru_nivcsw) * SB_US_PER_SEC / static_cast<double>(lv_us_cpu));
    ms_stats_print(pv_trace, la_line);
    if ((pp_value != NULL) && (strcasecmp(pp_value, "delta") == 0)) {
        if (lv_prev_ok) {
            long long lv_curr = lv_usage.ru_utime.tv_sec * SB_US_PER_SEC +
                                lv_usage.ru_utime.tv_usec;
            long long lv_prev = lv_usage_prev.ru_utime.tv_sec * SB_US_PER_SEC +
                                lv_usage_prev.ru_utime.tv_usec;
            long long lv_delta_cpu = lv_curr - lv_prev;
            long long lv_delta_wc = (lv_t.tv_sec * SB_US_PER_SEC +
                                     lv_t.tv_usec) -
                                    (lv_t_prev.tv_sec * SB_US_PER_SEC +
                                     lv_t_prev.tv_usec);
            long lv_nvcsw = lv_usage.ru_nvcsw - lv_usage_prev.ru_nvcsw;
            long lv_nivcsw = lv_usage.ru_nivcsw - lv_usage_prev.ru_nivcsw;
            if (lv_delta_cpu) {
                sprintf(la_line, "delta voluntary context switches.............=%ld\n",
                        lv_nvcsw);
                ms_stats_print(pv_trace, la_line);
                sprintf(la_line, "delta involuntary context switches...........=%ld\n",
                        lv_nivcsw);
                ms_stats_print(pv_trace, la_line);
                sprintf(la_line, "delta voluntary context switches/sec (cpu)...=%.3f (%lld)\n",
                        static_cast<double>(lv_nvcsw) * SB_US_PER_SEC / static_cast<double>(lv_delta_cpu),
                        lv_delta_cpu);
                ms_stats_print(pv_trace, la_line);
                sprintf(la_line, "delta involuntary context switches/sec (cpu).=%.3f (%lld)\n",
                        static_cast<double>(lv_nivcsw) * SB_US_PER_SEC / static_cast<double>(lv_delta_cpu),
                        lv_delta_cpu);
                ms_stats_print(pv_trace, la_line);
                sprintf(la_line, "delta voluntary context switches/sec (wc)....=%.3f (%lld)\n",
                        static_cast<double>(lv_nvcsw) * SB_US_PER_SEC / static_cast<double>(lv_delta_wc),
                        lv_delta_wc);
                ms_stats_print(pv_trace, la_line);
                sprintf(la_line, "delta involuntary context switches/sec (wc)..=%.3f (%lld)\n",
                        static_cast<double>(lv_nivcsw) * SB_US_PER_SEC / static_cast<double>(lv_delta_wc),
                        lv_delta_wc);
                ms_stats_print(pv_trace, la_line);
            }
        }
    }
    memcpy(&lv_usage_prev, &lv_usage, sizeof(lv_usage));
    lv_t_prev.tv_sec = lv_t.tv_sec;
    lv_t_prev.tv_usec = lv_t.tv_usec;
    lv_prev_ok = true;
}

void ms_stats_print(bool pv_trace, char *pp_line) {
    const char    *WHERE = "ms_stats";

    if (pv_trace)
        trace_where_printf(WHERE, pp_line);
    else
        printf("%s-%d: %s", ga_ms_su_pname, getpid(), pp_line);
}

//
// Purpose: clear current transid
//
void ms_transid_clear(MS_Mon_Transid_Type  pv_transid,
                      MS_Mon_Transseq_Type pv_startid) {
    const char *WHERE = "ms_transid_clear";
    int         lv_tmliberr;

    if (gv_ms_tmlib_callback != NULL) {
        if (gv_ms_trace_trans) {
            char la_transid[100];
            msg_util_format_transid(la_transid, pv_transid);
            trace_where_printf(WHERE, "TRANSID-CLEAR_TX, transid=%s\n",
                               la_transid);
        }
        // ignore any error outcome
        lv_tmliberr = gv_ms_tmlib_callback(TMLIB_FUN_CLEAR_TX,
                                           pv_transid,
                                           NULL);
        if (gv_ms_trace_trans && lv_tmliberr)
            trace_where_printf(WHERE, "TRANSID-CLEAR_TX, tmliberr=%d\n",
                               lv_tmliberr);
    } else if (gv_ms_tmlib2_callback != NULL) {
        if (gv_ms_trace_trans) {
            char la_startid[100];
            char la_transid[100];
            msg_util_format_transid(la_transid, pv_transid);
            msg_util_format_transseq(la_startid, pv_startid);
            trace_where_printf(WHERE, "TRANSID-CLEAR_TX, transid=%s, startid=%s\n",
                               la_transid, la_startid);
        }
        // ignore any error outcome
        lv_tmliberr = gv_ms_tmlib2_callback(TMLIB_FUN_CLEAR_TX,
                                            pv_transid,
                                            NULL,
                                            pv_startid,
                                            NULL);
        if (gv_ms_trace_trans && lv_tmliberr)
            trace_where_printf(WHERE, "TRANSID-CLEAR_TX, tmliberr=%d\n",
                               lv_tmliberr);
    }
}

//
// Purpose: return current transid
//
int ms_transid_get(bool                  pv_supp,
                   bool                  pv_trace,
                   MS_Mon_Transid_Type  *pp_transid,
                   MS_Mon_Transseq_Type *pp_startid) {
    const char *WHERE = "ms_transid_get";
    int         lv_tmliberr;

    if (pv_supp) {
        if (gv_ms_trace_trans || pv_trace)
            trace_where_printf(WHERE, "TRANSID-GET_TX, NOT called - suppressed\n");
        TRANSID_SET_NULL((*pp_transid));
        TRANSSEQ_SET_NULL((*pp_startid));
        lv_tmliberr = XZFIL_ERR_OK;
    } else if (gv_ms_tmlib_callback != NULL) {
        TRANSID_SET_NULL((*pp_transid));
        lv_tmliberr = gv_ms_tmlib_callback(TMLIB_FUN_GET_TX,
                                           *pp_transid,
                                           pp_transid);
        if (gv_ms_trace_trans || pv_trace) {
            char la_transid[100];
            msg_util_format_transid(la_transid, *pp_transid);
            trace_where_printf(WHERE, "TRANSID-GET_TX, transid=%s, tmliberr=%d\n",
                               la_transid, lv_tmliberr);
        }
    } else if (gv_ms_tmlib2_callback != NULL) {
        TRANSID_SET_NULL((*pp_transid));
        TRANSSEQ_SET_NULL((*pp_startid));
        lv_tmliberr = gv_ms_tmlib2_callback(TMLIB_FUN_GET_TX,
                                            *pp_transid,
                                            pp_transid,
                                            *pp_startid,
                                            pp_startid);
        if (gv_ms_trace_trans || pv_trace) {
            char la_startid[100];
            char la_transid[100];
            msg_util_format_transid(la_transid, *pp_transid);
            msg_util_format_transseq(la_startid, *pp_startid);
            trace_where_printf(WHERE, "TRANSID-GET_TX, transid=%s, startid=%s, tmliberr=%d\n",
                               la_transid, la_startid, lv_tmliberr);
        }
    } else {
        if (gv_ms_trace_trans || pv_trace)
            trace_where_printf(WHERE, "TRANSID-GET_TX, NOT called - no callback\n");
        TRANSID_SET_NULL((*pp_transid));
        TRANSSEQ_SET_NULL((*pp_startid));
        lv_tmliberr = XZFIL_ERR_OK;
    }
    return lv_tmliberr;
}

//
// Purpose: register current transid
//
int ms_transid_reg(MS_Mon_Transid_Type   pv_transid,
                   MS_Mon_Transseq_Type pv_startid) {
    const char *WHERE = "ms_transid_reg";
    int         lv_tmliberr;

    SB_util_assert((gv_ms_tmlib_callback != NULL) || (gv_ms_tmlib2_callback != NULL));
    if (gv_ms_tmlib_callback != NULL) {
        if (gv_ms_trace_trans) {
            char la_transid[100];
            msg_util_format_transid(la_transid, pv_transid);
            trace_where_printf(WHERE, "TRANSID-REG_TX, transid=%s\n",
                               la_transid);
        }
        lv_tmliberr = gv_ms_tmlib_callback(TMLIB_FUN_REG_TX,
                                           pv_transid,
                                           NULL);
        if (gv_ms_trace_trans && lv_tmliberr)
            trace_where_printf(WHERE, "TRANSID-REG_TX, tmliberr=%d\n", lv_tmliberr);
    } else if (gv_ms_tmlib2_callback != NULL) {
        if (gv_ms_trace_trans) {
            char la_startid[100];
            char la_transid[100];
            msg_util_format_transid(la_transid, pv_transid);
            msg_util_format_transseq(la_startid, pv_startid);
            trace_where_printf(WHERE, "TRANSID-REG_TX, transid=%s, startid=%s\n",
                               la_transid, la_startid);
        }
        lv_tmliberr = gv_ms_tmlib2_callback(TMLIB_FUN_REG_TX,
                                            pv_transid,
                                            NULL,
                                            pv_startid,
                                            NULL);
        if (gv_ms_trace_trans && lv_tmliberr)
            trace_where_printf(WHERE, "TRANSID-REG_TX, tmliberr=%d\n", lv_tmliberr);
    }
    return lv_tmliberr;
}

//
// Purpose: reinstate transid
//
int ms_transid_reinstate(MS_Mon_Transid_Type  pv_transid,
                         MS_Mon_Transseq_Type pv_startid) {
    const char *WHERE = "ms_transid_reinstate";
    int         lv_tmliberr;

    if (gv_ms_tmlib_callback != NULL) {
        if (gv_ms_trace_trans) {
            char la_transid[100];
            msg_util_format_transid(la_transid, pv_transid);
            trace_where_printf(WHERE, "TRANSID-REINSTATE_TX, transid=%s\n",
                               la_transid);
        }
        lv_tmliberr = gv_ms_tmlib_callback(TMLIB_FUN_REINSTATE_TX,
                                           pv_transid,
                                           NULL);
        if (gv_ms_trace_trans && lv_tmliberr) {
            trace_where_printf(WHERE, "TRANSID-REINSTATE_TX, tmliberr=%d\n",
                               lv_tmliberr);
        }
    } else if (gv_ms_tmlib2_callback != NULL) {
        if (gv_ms_trace_trans) {
            char la_startid[100];
            char la_transid[100];
            msg_util_format_transid(la_transid, pv_transid);
            msg_util_format_transseq(la_startid, pv_startid);
            trace_where_printf(WHERE, "TRANSID-REINSTATE_TX, transid=%s, startid=%s\n",
                               la_transid, la_startid);
        }
        lv_tmliberr = gv_ms_tmlib2_callback(TMLIB_FUN_REINSTATE_TX,
                                            pv_transid,
                                            NULL,
                                            pv_startid,
                                            NULL);
        if (gv_ms_trace_trans && lv_tmliberr) {
            trace_where_printf(WHERE, "TRANSID-REINSTATE_TX, tmliberr=%d\n",
                               lv_tmliberr);
        }
    } else
        lv_tmliberr = XZFIL_ERR_OK;
    return lv_tmliberr;
}

//
// Purpose: fill phandle
//
void ms_util_fill_phandle_name(SB_Phandle_Type *pp_phandle,
                               char            *pp_name,
                               int              pv_nid,
                               int              pv_pid
#ifdef SQ_PHANDLE_VERIFIER
                              ,SB_Verif_Type    pv_verif
#endif
                              ) {
#ifdef SQ_PHANDLE_VERIFIER
    char       *lp_p;
#endif
    SB_Phandle *lp_phandle;
    int         lv_len;

    lp_phandle = reinterpret_cast<SB_Phandle *>(pp_phandle);
#ifdef SQ_PHANDLE_VERIFIER
    lp_p = strchr(pp_name, ':');
    if (lp_p == NULL)
        lv_len = static_cast<int>(strlen(pp_name));
    else
        lv_len = static_cast<int>(lp_p - pp_name);
#else
    lv_len = static_cast<int>(strlen(pp_name));
#endif
    memset(pp_phandle, 0, sizeof(SB_Phandle));
    lp_phandle->iv_type = PH_NAMED;
    lp_phandle->iv_vers = SB_PHANDLE_VERS;
    lp_phandle->iv_len = static_cast<char>(sizeof(SB_Phandle));
    lp_phandle->iv_name_len = static_cast<char>(lv_len);
    // strncpy may cause overlap problem, so check before copy
    if (lv_len < SB_PHANDLE_NAME_SIZE)
        memcpy(reinterpret_cast<char *>(&lp_phandle->ia_name), pp_name, lv_len);
    else
        memcpy(&lp_phandle->ia_name, pp_name, SB_PHANDLE_NAME_SIZE);
    lp_phandle->iv_nid = pv_nid;
    lp_phandle->iv_pid = pv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_phandle->iv_verifier = pv_verif;
#endif
}

//
// Purpose: fill phandle
//
void ms_util_fill_phandle_oid(SB_Phandle_Type *pp_phandle, int pv_oid) {
    SB_Phandle *lp_phandle = reinterpret_cast<SB_Phandle *>(pp_phandle);
    lp_phandle->iv_oid = pv_oid;
}

#ifdef SQ_PHANDLE_VERIFIER
void ms_util_name_seq(const char *pp_name_seq,
                      char       *pp_name,
                      size_t      pv_name_len,
                      Verifier_t *pp_verif) {
    const char *lp_p;
    int         lv_len;
    int         lv_name_len;

    lp_p = strchr(pp_name_seq, ':');
    if (lp_p == NULL) {
        *pp_verif = -1;
        ms_util_string_copy(pp_name,
                            pv_name_len,
                            pp_name_seq);
    } else {
        lv_name_len = static_cast<int>(pv_name_len);
        sscanf(&lp_p[1], "%d", pp_verif);
        lv_len = static_cast<int>(lp_p - pp_name_seq);
        if ((lv_len + 1) <= lv_name_len) {
            memcpy(pp_name, pp_name_seq, lv_len);
            pp_name[lv_len] = '\0';
        } else {
            SB_util_assert_ine(lv_name_len, 0);
            memcpy(pp_name, pp_name_seq, lv_name_len - 1);
            pp_name[lv_name_len] = '\0';
        }
    }
}

//
// Purpose: get seq #
//
bool ms_util_name_seq_get(char           *pp_name_seq,
                          char          **ppp_seq,
                          SB_Verif_Type  *pp_verif) {
    char *lp_p;
    bool  lv_ret;

    lp_p = strchr(pp_name_seq, ':');
    if (lp_p == NULL)
        lv_ret = false;
    else {
        lp_p++;
        lv_ret = true;
        if (ppp_seq != NULL)
            *ppp_seq = lp_p;
        if (pp_verif != NULL)
            sscanf(lp_p, PFVY, pp_verif);
    }

    return lv_ret;
}
#endif

//
// Purpose: map process-type to component-id
//
int ms_util_map_ptype_to_compid(int pv_ms_su_ptype) {
    int lv_compid;

    switch (pv_ms_su_ptype) {
    case ProcessType_TSE:
        lv_compid = SQEVL_TSE;
        break;
    case ProcessType_DTM:
        lv_compid = SQEVL_DTM;
        break;
    case ProcessType_ASE:
        lv_compid = SQEVL_ASE;
        break;
    case ProcessType_AMP:
        lv_compid = SQEVL_AMP;
        break;
    default:
        lv_compid = 0; // invalid
        break;
    }
    return lv_compid;
}

//
// Purpose: clear string
//
void ms_util_string_clear(char   *pp_dest,
                          size_t  pv_dest_size) {
    pp_dest[0] = '\0';
    pv_dest_size = pv_dest_size; // touch
}

//
// Purpose: copy string
//
void ms_util_string_copy(char       *pp_dest,
                         size_t      pv_dest_size,
                         const char *pp_src) {
    strncpy(pp_dest, pp_src, pv_dest_size - 1);
    pp_dest[pv_dest_size - 1] = '\0'; \
}

