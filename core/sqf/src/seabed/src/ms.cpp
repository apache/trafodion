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
//
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <malloc.h>
#include <linux/unistd.h> // gettid

#include <sys/stat.h>
#include <sys/time.h>

#include "common/evl_sqlog_eventnum.h"

#include "seabed/fserr.h"
#include "seabed/log.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "apictr.h"
#include "buf.h"
#include "chk.h"
#include "compq.h"
#include "env.h"
#include "mserr.h"
#include "mserrmsg.h"
#include "mslabelmapsx.h"
#include "msmon.h"
#include "mstrace.h"
#include "msutil.h"
#include "msvars.h"
#include "msx.h"
#include "pctlx.h"
#include "props.h"
#include "qid.h"
#include "smap.h"
#include "threadtlsx.h"
#include "timerx.h"
#include "util.h"
#include "utracex.h"


#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

extern "C" const char *libsbms_vers2_str();
extern void            SB_thread_main();

typedef void (*Prof_Flush)(const char *,int);
typedef union {
    Prof_Flush  iflush;
    void       *ipflush;
} Prof_Flush_Type;
typedef void (*Prof_Mark)();
typedef union {
    Prof_Mark   imark;
    void       *ipmark;
} Prof_Mark_Type;
typedef void (*Prof_Reset)();
typedef union {
    Prof_Reset  ireset;
    void       *ipreset;
} Prof_Reset_Type;

// from NSK: dmsghi
enum { MSG_MAXREPLYCTRLSIZE    =  900 };
enum { MSG_MAXREQCTRLSIZE      = 1100 };
enum { MSG_MINREPLYCTRLSIZE    =    8 };
enum { MSG_MINREQCTRLSIZE      =    8 };

static const char     *gp_config_file            = "ms.env";
static char           *gp_config_arg_file        = NULL;

// forwards
static void ms_fsdone_key_dtor(void *pp_queue);
static void ms_ldone_key_dtor(void *pp_queue);
#ifdef SQ_PHANDLE_VERIFIER
static void ms_recv_q_proc_death_md(MS_Md_Type    *pp_md,
                                    int            pv_nid,
                                    int            pv_pid,
                                    SB_Verif_Type  pv_verif,
                                    bool           pv_use_stream,
                                    void          *pp_stream);
#else
static void ms_recv_q_proc_death_md(MS_Md_Type *pp_md,
                                    int         pv_nid,
                                    int         pv_pid,
                                    bool        pv_use_stream,
                                    void       *pp_stream);
#endif

// globals
SB_Comp_Queue          gv_fs_comp_q(QID_FS_COMP, "q-fs-comp-md");
static SB_Comp_Queue   gv_ms_abandon_comp_q(QID_ABANDON_COMP, "q-abandon-comp-md");
bool                   gv_ms_attach             = false;
MS_Buf_Mgr_Type        gv_ms_buf_mgr            = {
    {malloc}, {free},
    {malloc}, {free}
};
int                    gv_ms_buf_options        = 0;
bool                   gv_ms_calls_ok           = false;
bool                   gv_ms_client_only        = false;
int                    gv_ms_conn_idle_timeout  = 0; // disabled
bool                   gv_ms_conn_reuse         = true;
static int             gv_ms_disable_wait       = 0;
int                    gv_ms_disc_sem           = 50;
bool                   gv_ms_disc_sem_rob       = false;
bool                   gv_ms_disc_sem_stats     = false;
int                    gv_ms_enable_messages    = 0;
static SB_Props        gv_ms_env(true);
static bool            gv_ms_env_loaded         = false;
SB_Ms_Tl_Event_Mgr     gv_ms_event_mgr;
#ifdef USE_EVENT_REG
static bool            gv_ms_event_wait_abort   = true;
#endif
int                    gv_ms_fd_stderr          = -1;
int                    gv_ms_fd_stdout          = -1;
SB_Ts_Lmap             gv_ms_fsdone_map("map-ms-fsdone"); // used by IC
static int             gv_ms_fsdone_tls_inx      =
                         SB_create_tls_key(ms_fsdone_key_dtor, "ms-fs-compq");
SB_Ts_D_Queue          gv_ms_hold_q(QID_HOLD, "q-hold-md");
bool                   gv_ms_ic_ibv             = false;
bool                   gv_ms_inited             = false;
bool                   gv_ms_init_phandle       = true;
bool                   gv_ms_init_test          = false;
static SB_Comp_Queue   gv_ms_ldone_comp_q(QID_LDONE_COMP, "q-ldone-comp-md");
static int             gv_ms_ldone_tls_inx      =
                         SB_create_tls_key(ms_ldone_key_dtor, "ms-compq");
MS_Lim_Queue_Cb_Type   gv_ms_lim_cb             = 0;
SB_Recv_Queue          gv_ms_lim_q("lim-q-md");
int                    gv_ms_max_phandles       = 0;
bool                   gv_ms_mon_calls_ok       = false;
bool                   gv_ms_msg_timestamp      = false;
bool                   gv_ms_pre_inited         = false;
bool                   gv_ms_process_comp       = false;
SB_Recv_Queue          gv_ms_recv_q("recv-q-md");
bool                   gv_ms_recv_q_proc_death  = false;
bool                   gv_ms_shutdown_called    = false;
bool                   gv_ms_shutdown_fast      = true;
bool                   gv_ms_trans_sock         = false;
double                 gv_ms_wtime_adj;

enum { MAX_RETRIES  =   5 };

static SB_Lmap         gv_ms_abandon_map("map-ms-abandon");
bool                   gv_ms_assert_chk         = false;
bool                   gv_ms_assert_chk_send    = false;
SB_Ts_Lmap             gv_ms_ldone_map("map-ms-ldone"); // used by IC
static SB_Smap         gv_ms_phandle_map("map-ms-phandle");

void __attribute__((constructor)) __msg_init(void);
void __attribute__((destructor))  __msg_fini(void);

SB_Trans::Md_Table_Entry_Mgr SB_Trans::Msg_Mgr::cv_md_table_entry_mgr;
SB_Trans::Md_Table_Mgr       SB_Trans::Msg_Mgr::cv_md_table("tablemgr-MD",
                                                            SB_Table_Mgr_Alloc::ALLOC_FIFO,
                                                            SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                                            &cv_md_table_entry_mgr,
                                                            4096, 1024); // cap-init, cap-inc
int                          SB_Trans::Msg_Mgr::cv_md_table_inx = SB_Trans::Msg_Mgr::init();

//
// forwards
//
static void   ms_fifo_fd_restore();
static void   ms_fifo_fd_save();
static bool   ms_is_mon_number(char *pp_arg);
static void   ms_ldone_key_dtor(void *pp_queue);
static void   ms_ldone_key_dtor_lock(void *pp_queue, bool pv_lock);
static void   msg_init_env_load();
static void   msg_init_env_load_exe(const char *pp_exe, SB_Props *pp_env);
static void   msg_init_trace_com(const char  *pp_where,
                                 int          pv_argc,
                                 char       **ppp_argv);

//
// Implementation notes:
//   Buffers are NOT checked for valid address.
//   In linux kernel mode, it's possible to use the macro access_of(),
//   but the macro is fairly crude in the checks that it makes.
//   However, there is an assumption that the addresses will
//   be checked by MPI at some point. The only real downside
//   to this approach, is that a failure may be latently reported
//   (e.g. in the case of MSG_LINK_ with a bad recv buffer address).
//


//
// Purpose: complete abandon (callback-target)
//
void ms_abandon_cbt(MS_Md_Type *pp_md, bool pv_add) {
    const char *WHERE = "ms_abandon_cbt";

    if (pv_add) {
        gv_ms_abandon_comp_q.add(&pp_md->iv_link);
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "setting LCAN, msgid=%d\n",
                               pp_md->iv_link.iv_id.i);
    } else {
        // remove from abandon-q
        gv_ms_abandon_comp_q.remove_list(&pp_md->iv_link);
    }
}

//
// Purpose:
//
void ms_fifo_setup(int pv_orig_fd, char *pp_fifo_name, bool pv_remap_fd) {
    const char *WHERE = "ms_fifo_setup";
    int         lv_err;
    int         lv_fifo_fd;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "opening fifo to monitor, fifo=%s, fd=%d\n",
                           pp_fifo_name, pv_orig_fd);

    // Open the fifo for writing.
    lv_fifo_fd = open(pp_fifo_name, O_WRONLY);
    if (lv_fifo_fd == -1) {
        if (gv_ms_trace)
            trace_where_printf("fifo open error, fifo=%s, errno=%d\n",
                               pp_fifo_name, errno);
    } else {
        if (pv_remap_fd) {
            // Remap fifo file descriptor
            // Close unneeded fifo file descriptor.
            lv_err = close(pv_orig_fd);
            if (lv_err == -1) {
                if (gv_ms_trace)
                    trace_where_printf(WHERE, "fifo original close error, fd=%d, errno=%d\n",
                                       pv_orig_fd, errno);
            }

            lv_err = dup2(lv_fifo_fd, pv_orig_fd);
            if (lv_err == -1) {
                if (gv_ms_trace)
                    trace_where_printf(WHERE, "fifo dup2 error, old-fd=%d, new-fd=%d, errno=%d\n",
                                       lv_fifo_fd, pv_orig_fd, errno);
            } else {
                lv_err = close(lv_fifo_fd);
                if (lv_err == -1) {
                    if (gv_ms_trace)
                        trace_where_printf(WHERE, "fifo close error, fifo-fd=%d, errno=%d\n",
                                           lv_fifo_fd, errno);
                }
            }
        }
    }
}

//
// Purpose: Restore standard fds
//
void ms_fifo_fd_restore() {
    if (gv_ms_fd_stdout != -1)
        dup2(gv_ms_fd_stdout, 1);
    if (gv_ms_fd_stderr != -1)
        dup2(gv_ms_fd_stderr, 1);
}

//
// Purpose: Save standard fds
//
void ms_fifo_fd_save() {
    gv_ms_fd_stdout = dup(1);
    gv_ms_fd_stderr = dup(2);
}

//
// Purpose: Set requeue in md
//
void ms_msg_set_requeue(const char *pp_where, int pv_msgid, bool pv_requeue) {
    MS_Md_Type *lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, pp_where);
    lp_md->out.iv_requeue = pv_requeue;
}

void ms_buf_free(void *pp_buf) {
    const char *WHERE = "msg_buf_free";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE,
                           "ENTER buf=%p\n", pp_buf);
    gv_ms_buf_mgr.ufi.ifree(pp_buf);
}

void *ms_buf_malloc(size_t pv_size) {
    const char *WHERE = "msg_buf_malloc";

    void *lp_buf = gv_ms_buf_mgr.uai.ialloc(pv_size);
    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE,
                           "EXIT size=" PFSZ ", buf=%p\n",
                           pv_size,
                           lp_buf);
    return lp_buf;
}

//
// Purpose: set buf manager
// If tracing, call through tracer.
// If no tracing, call directly.
//
void ms_buf_trace_change() {
    if (gv_ms_trace_enable && gv_ms_trace_alloc) {
        gv_ms_buf_mgr.ua.ialloc = ms_buf_malloc;
        gv_ms_buf_mgr.uf.ifree = ms_buf_free;
    } else {
        gv_ms_buf_mgr.ua.ialloc = gv_ms_buf_mgr.uai.ialloc;
        gv_ms_buf_mgr.uf.ifree = gv_ms_buf_mgr.ufi.ifree;
    }
}

//
// Purpose: set options
//
SB_Export short msg_buf_options(int pv_options) {
    const char *WHERE = "msg_buf_options";
    SB_API_CTR (lv_zctr, MSG_BUF_OPTIONS);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER options=0x%x\n", pv_options);
    short lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_inited)
        lv_fserr = XZFIL_ERR_INVALIDSTATE;
    else if (pv_options & ~MS_BUF_OPTION_ALL) {
        // reserved bits turned on - error
        lv_fserr = XZFIL_ERR_BOUNDSERR;
    }
    if (lv_fserr == XZFIL_ERR_OK)
        gv_ms_buf_options = pv_options;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: read-ctrl
//
SB_Export short msg_buf_read_ctrl(int     pv_msgid,
                                  short **ppp_reqctrl,
                                  int    *pp_bytecount,
                                  int     pv_clear) {
    const char *WHERE = "msg_buf_read_ctrl";
    MS_Md_Type *lp_md;
    SB_API_CTR (lv_zctr, MSG_BUF_READ_CTRL);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER msgid=%d, reqC=%p, bytecount=%p, clear=%d\n",
                           pv_msgid, pfp(ppp_reqctrl),
                           pfp(pp_bytecount), pv_clear);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, WHERE);
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    int lv_bytecount = lp_md->out.iv_recv_ctrl_size;
    if (pp_bytecount != NULL)
        *pp_bytecount = lv_bytecount;
    *ppp_reqctrl = reinterpret_cast<short *>(lp_md->out.ip_recv_ctrl);
    if (pv_clear) {
        lp_md->out.ip_recv_ctrl = NULL;
        lp_md->out.iv_recv_ctrl_size = 0;
    }
    if (gv_ms_trace_data) {
        trace_where_printf(WHERE,
                           "reqC=%p, bytecount=%d\n",
                           pfp(*ppp_reqctrl), lv_bytecount);
        trace_print_data(*ppp_reqctrl, lv_bytecount, gv_ms_trace_data_max);
    }
    short lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: read-data
//
SB_Export short msg_buf_read_data(int    pv_msgid,
                                  char **ppp_buffer,
                                  int   *pp_bytecount,
                                  int    pv_clear) {
    const char *WHERE = "msg_buf_read_data";
    MS_Md_Type *lp_md;
    SB_API_CTR (lv_zctr, MSG_BUF_READ_DATA);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER msgid=%d, buffer=%p, bytecount=%p, clear=%d\n",
                           pv_msgid, pfp(ppp_buffer),
                           pfp(pp_bytecount), pv_clear);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, WHERE);
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    int lv_bytecount = lp_md->out.iv_recv_data_size;
    if (pp_bytecount != NULL)
        *pp_bytecount = lv_bytecount;
    *ppp_buffer = lp_md->out.ip_recv_data;
    if (pv_clear) {
        lp_md->out.ip_recv_data = NULL;
        lp_md->out.iv_recv_data_size = 0;
    }
    if (gv_ms_trace_data) {
        trace_where_printf(WHERE,
                           "buffer=%p, bytecount=%d\n",
                           *ppp_buffer, lv_bytecount);
        trace_print_data(*ppp_buffer, lv_bytecount, gv_ms_trace_data_max);
    }
    short lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: read-data
// (similar to BMSG_READDATA_ except no abandon check)
//
short msg_buf_read_data_int(int     pv_msgid,
                            char   *pp_reqdata,
                            int     pv_bytecount) {
    const char *WHERE = "msg_buf_read_data_int";
    MS_Md_Type *lp_md;
    int         lv_rc;
    SB_API_CTR (lv_zctr, BMSG_READDATA_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER msgid=%d, reqD=%p, bytecount=%d\n",
                           pv_msgid, pp_reqdata, pv_bytecount);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, WHERE);
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
        SB_Trans::Trans_Stream::recv_copy(WHERE,
                                          &lp_md->out.ip_recv_data,
                                          lp_md->out.iv_recv_data_size,
                                          pp_reqdata,
                                          pv_bytecount,
                                          &lv_rc,
                                          false);
    if (gv_ms_trace_data) {
        trace_where_printf(WHERE,
                           "reqD=%p, bytecount=%d\n",
                           pp_reqdata, lv_rc);
        trace_print_data(pp_reqdata, lv_rc, gv_ms_trace_data_max);
    }
    short lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}


//
// Purpose: register buffer
//
SB_Export short msg_buf_register(MS_Buf_Alloc_Cb_Type pv_callback_alloc,
                                 MS_Buf_Free_Cb_Type  pv_callback_free) {
    const char *WHERE = "msg_buf_register";
    short       lv_fserr;
    SB_API_CTR (lv_zctr, MSG_BUF_REGISTER);

    if (gv_ms_inited)
        lv_fserr = XZFIL_ERR_INVALIDSTATE;
    else {
        lv_fserr = XZFIL_ERR_OK;
        gv_ms_buf_mgr.ua.ialloc = pv_callback_alloc;
        gv_ms_buf_mgr.uf.ifree = pv_callback_free;
        gv_ms_buf_mgr.uai.ialloc = pv_callback_alloc;
        gv_ms_buf_mgr.ufi.ifree = pv_callback_free;
    }
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return lv_fserr;
}

//
// Purpose: debug hook
//
SB_Export void msg_debug_hook(const char *pp_who, const char *pp_fname) {
    char         la_buf[500];
    char        *lp_p;
    int          lv_enable;
    int          lv_err;
    struct stat  lv_stat;
    SB_API_CTR  (lv_zctr, MSG_DEBUG_HOOK);

    lp_p = getenv(gp_ms_env_hook_enable);
    if (lp_p == NULL)
        return;
    lv_enable = atoi(lp_p);
    if (!lv_enable)
        return;
    lv_err = stat(pp_fname, &lv_stat);
    if (lv_err == -1) {
        lp_p = getcwd(la_buf, sizeof(la_buf));
        printf("%s: create file %s/%s to continue\n",
               pp_who, lp_p, pp_fname);
        printf("%s: slave pid=%d\n", pp_who, getpid());
        for (;;) {
            lv_err = stat(pp_fname, &lv_stat);
            if (lv_err == 0) {
                printf("%s: %s detected - continuing\n", pp_who, pp_fname);
                break;
            }
            sleep(1); // msg_debug_hook
        }
    }
}

//
// Purpose: set lim-queue
//
SB_Export void msg_enable_lim_queue(MS_Lim_Queue_Cb_Type pv_cb) {
    const char *WHERE = "msg_enable_lim_queue";
    SB_API_CTR (lv_zctr, MSG_ENABLE_LIM_QUEUE);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    gv_ms_lim_cb = pv_cb;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// Purpose: set open-cleanup
//
SB_Export int msg_enable_open_cleanup() {
    const char *WHERE = "msg_enable_open_cleanup";
    int         lv_fserr;
    SB_API_CTR (lv_zctr, MSG_ENABLE_OPEN_CLEANUP);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    lv_fserr = ms_od_cleanup_enable();
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return lv_fserr;
}

//
// Purpose: set priority-queue
//
SB_Export void msg_enable_priority_queue() {
    const char *WHERE = "msg_enable_priority_queue";
    SB_API_CTR (lv_zctr, MSG_ENABLE_PRIORITY_QUEUE);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    gv_ms_recv_q.set_priority_queue(true);
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// Purpose: set proc-death on recv-queue
//
SB_Export void msg_enable_recv_queue_proc_death() {
    const char *WHERE = "msg_enable_recv_queue_proc_death";
    SB_API_CTR (lv_zctr, MSG_ENABLE_RECV_QUEUE_PROC_DEATH);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    gv_ms_recv_q_proc_death = true;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// Purpose: Check mpi error - pathdown ok
//
void ms_err_check_mpi_pathdown_ok(const char *pp_where,
                                  const char *pp_msg,
                                  short       pv_fserr) {
    switch (pv_fserr) {
    case XZFIL_ERR_OK:
    case XZFIL_ERR_DEVDOWN:
    case XZFIL_ERR_PATHDOWN:
        break;

    default:
        if (gv_ms_trace_params)
            trace_where_printf(pp_where, "%s, fserr=%d\n", pp_msg, pv_fserr);
        SB_util_assert((pv_fserr == XZFIL_ERR_OK) ||
                       (pv_fserr == XZFIL_ERR_DEVDOWN) ||
                       (pv_fserr == XZFIL_ERR_PATHDOWN));
        break;
    }
}

//
// Purpose: return an ms error
//
static short ms_err_mpi_rtn(const char *pp_where, int pv_mpierr) {
    return ms_err_rtn(ms_err_mpi_to_fserr(pp_where, pv_mpierr));
}

//
// Purpose: trace msg and return an ms error
//
short ms_err_mpi_rtn_msg(const char *pp_where,
                         const char *pp_msg,
                         int         pv_mpierr) {
    return ms_err_rtn_msg(pp_where, pp_msg,
                          ms_err_mpi_to_fserr(pp_where, pv_mpierr));
}

//
// Purpose: trace msg and return an ms error
//
short ms_err_mpi_rtn_msg_noassert(const char *pp_where,
                                  const char *pp_msg,
                                  int         pv_mpierr) {
    return ms_err_rtn_msg_noassert(pp_where, pp_msg,
                                   ms_err_mpi_to_fserr(pp_where, pv_mpierr));
}

//
// Purpose: fatal error
//
short ms_err_mpi_rtn_msg_fatal(const char *pp_where,
                               const char *pp_msg,
                               int         pv_mpierr) {
    short lv_fserr = ms_err_mpi_to_fserr(pp_where, pv_mpierr);
    return ms_err_rtn_msg_fatal(pp_where, pp_msg, lv_fserr, true);
}

//
// Purpose: free recv buffers
//
void ms_free_recv_bufs(MS_Md_Type *pp_md) {
    if (pp_md->out.ip_recv_ctrl != NULL) {
        MS_BUF_MGR_FREE(pp_md->out.ip_recv_ctrl);
        pp_md->out.ip_recv_ctrl = NULL;
    }
    if (pp_md->out.ip_recv_data != NULL) {
        MS_BUF_MGR_FREE(pp_md->out.ip_recv_data);
        pp_md->out.ip_recv_data = NULL;
    }
}

//
// Purpose: get completion queue
//
SB_Comp_Queue *ms_fsdone_get_comp_q(bool pv_ts) {
    SB_Comp_Queue *lp_queue;
    int            lv_status;

    if (pv_ts) {
        lp_queue =
          static_cast<SB_Comp_Queue *>(SB_Thread::Sthr::specific_get(gv_ms_fsdone_tls_inx));
        if (lp_queue == NULL) {
            lp_queue =
              new SB_Comp_Queue(QID_FS_TS_COMP, "q-fsdone-ts-comp-md");
            SB_LML_Type *lp_info = new SB_LML_Type;
            lp_info->iv_id.l = reinterpret_cast<long>(lp_queue);
            gv_ms_fsdone_map.put(lp_info);
            lv_status = SB_Thread::Sthr::specific_set(gv_ms_fsdone_tls_inx, lp_queue);
            SB_util_assert_ieq(lv_status, 0);
        }
    } else
        lp_queue = &gv_fs_comp_q;

    return lp_queue;
}

//
// cleanup TS completion-queue
//
static void ms_fsdone_key_dtor(void *pp_queue) {
    SB_Comp_Queue *lp_queue = static_cast<SB_Comp_Queue *>(pp_queue);

    if (lp_queue != NULL) {
        SB_LML_Type *lp_info =
          static_cast<SB_LML_Type *>(gv_ms_fsdone_map.remove_lock(reinterpret_cast<long>(lp_queue), false));
        if (lp_info != NULL) {
            // only delete if it hasn't already been deleted
            delete lp_info;
            delete lp_queue;
        }
    }
}

//
// Purpose: map FS error to results error
//
static void ms_fserr_to_results(short pv_fserr, MS_Result_Type *pp_results) {
    // TODO revisit
    switch (pv_fserr) {
    case XZFIL_ERR_OK:
        break;
    case XZFIL_ERR_DEVDOWN:
    case XZFIL_ERR_PATHDOWN:
        pp_results->rr_ctrlsize = 0;
        pp_results->rr_datasize = 0;
        pp_results->rrerr_updatedestb = 1;
        pp_results->rrerr_countitb = 1;
        pp_results->rrerr_startedb = 1;
        pp_results->rrerr_retryableb = 1;
        pp_results->rrerr_errorb = 1;
        break;
    default:
        pp_results->rr_ctrlsize = 0;
        pp_results->rr_datasize = 0;
        pp_results->rrerr_startedb = 1;
        pp_results->rrerr_errorb = 1;
        break;
    }
}

// Purpose: gather some helpful info
//
void ms_gather_info(const char *pp_where) {
    char la_host[HOST_NAME_MAX];
    int  lv_err;

    if (gv_ms_su_nid >= 0)
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(pp_where, "pname=%s, p-id=%d/%d/" PFVY "\n",
                           ga_ms_su_pname, gv_ms_su_nid, getpid(), gv_ms_su_verif);
#else
        trace_where_printf(pp_where, "pname=%s, nid=%d, pid=%d\n",
                           ga_ms_su_pname, gv_ms_su_nid, getpid());
#endif
    else
        trace_where_printf(pp_where, "pname=%s, pid=%d\n", ga_ms_su_pname, getpid());
    lv_err = gethostname(la_host, HOST_NAME_MAX);
    if (lv_err == -1)
        trace_where_printf(pp_where, "gethostname ret=%d\n", lv_err);
    else
        trace_where_printf(pp_where, "gethostname host=%s\n", la_host);

    FILE *lp_file = fopen("/proc/cpuinfo", "r");
    if (lp_file != NULL) {
        char         la_line[100];
        const char *lp_core = "core id\t";
        const char *lp_cores = "cpu cores\t";
        const char *lp_model = "model name\t";
        const char *lp_proc  = "processor\t";
        const char *lp_speed = "cpu MHz\t";
        int         lv_core_len = static_cast<int>(strlen(lp_core));
        int         lv_cores_len = static_cast<int>(strlen(lp_cores));
        int         lv_model_len = static_cast<int>(strlen(lp_model));
        int         lv_proc_len = static_cast<int>(strlen(lp_proc));
        int         lv_speed_len = static_cast<int>(strlen(lp_speed));
        int         lv_max_proc = -1;
        for (;;) {
            char *lp_s = fgets(la_line, sizeof(la_line), lp_file);
            if (lp_s == NULL)
                break;
#ifdef DEBUG_PRINT
            printf("/proc/cpuinfo=%s", la_line);
            fflush(stdout);
#else
            if ((memcmp(lp_s, lp_core, lv_core_len) == 0) ||
                (memcmp(lp_s, lp_cores, lv_cores_len) == 0) ||
                (memcmp(lp_s, lp_proc, lv_proc_len) == 0) ||
                (memcmp(lp_s, lp_model, lv_model_len) == 0) ||
                (memcmp(lp_s, lp_speed, lv_speed_len) == 0))
                trace_where_printf(pp_where, "/proc/cpuinfo=%s", lp_s);
#endif // !DEBUG_PRINT
            if (memcmp(lp_s, lp_proc, lv_proc_len) == 0) {
                char la_colon[100];
                char la_proc[100];
                int  lv_proc;
                sscanf(lp_s, "%s %s %d",
                       la_proc, la_colon, &lv_proc);
                if (lv_proc > lv_max_proc)
                    lv_max_proc = lv_proc;
            }
        }
        fclose(lp_file);
        if (lv_max_proc >= 0)
            lv_max_proc++;
        trace_where_printf(pp_where, "max processors=%d\n", lv_max_proc);
    }
}

//
// Purpose: check if monitor number
//
bool ms_is_mon_number(char *pp_arg) {
    int lv_inx;
    int lv_len;

    lv_len = static_cast<int>(strlen(pp_arg));

    if ((lv_len < 5) || (lv_len > 8))
        return false;
    for (lv_inx = 0; lv_inx < lv_len; lv_inx++)
        if (!isdigit(pp_arg[lv_inx]))
            return false;
    return true;
}

//
// Purpose: complete ldone (callback-target)
//
void ms_ldone_cbt(MS_Md_Type *pp_md) {
    SB_Comp_Queue *lp_comp_q;

    lp_comp_q = pp_md->ip_comp_q; // in case break called
    if (lp_comp_q != NULL)
        lp_comp_q->add(&pp_md->iv_link);

    if (gv_ms_process_comp) {
#ifdef USE_EVENT_REG
        gv_ms_event_mgr.set_event_reg(LDONE);
#else
        gv_ms_event_mgr.get_gmgr()->set_event(LDONE, &pp_md->iv_reply_done);
#endif
    } else
        pp_md->ip_mgr->set_event_atomic(LDONE, &pp_md->iv_reply_done);
}

//
// Purpose: get completion queue
//
SB_Comp_Queue *ms_ldone_get_comp_q() {
    int lv_status;

    if (gv_ms_process_comp)
        return &gv_ms_ldone_comp_q;
    SB_Comp_Queue *lp_queue =
      static_cast<SB_Comp_Queue *>(SB_Thread::Sthr::specific_get(gv_ms_ldone_tls_inx));
    if (lp_queue == NULL) {
        lp_queue = new SB_Comp_Queue(QID_LDONE_COMP, "q-ldone-comp-md");
        SB_LML_Type *lp_info = new SB_LML_Type;
        lp_info->iv_id.l = reinterpret_cast<long>(lp_queue);
        gv_ms_ldone_map.put(lp_info);
        lv_status = SB_Thread::Sthr::specific_set(gv_ms_ldone_tls_inx, lp_queue);
        SB_util_assert_ieq(lv_status, 0);
    }
    return lp_queue;
}

static void ms_ldone_key_dtor(void *pp_queue) {
    ms_ldone_key_dtor_lock(pp_queue, true);
}

static void ms_ldone_key_dtor_lock(void *pp_queue, bool pv_lock) {
    SB_Comp_Queue *lp_queue = static_cast<SB_Comp_Queue *>(pp_queue);

    if (lp_queue != NULL) {
        if (pv_lock)
            gv_ms_ldone_map.lock();
        SB_LML_Type *lp_info =
          static_cast<SB_LML_Type *>(gv_ms_ldone_map.remove_lock(reinterpret_cast<long>(lp_queue), false));
        if (lp_info != NULL) {
            // only delete if it hasn't already been deleted
            delete lp_info;
            delete lp_queue;
        }
        if (pv_lock)
            gv_ms_ldone_map.unlock();
    }
}

static void ms_ldone_shutdown() {
    void *lp_queue;
    bool  lv_done;

    lv_done = false;
    do {
        gv_ms_ldone_map.lock();
        SB_Lmap_Enum *lp_enum = gv_ms_ldone_map.keys();
        if (lp_enum->more()) {
            SB_LML_Type *lp_info = static_cast<SB_LML_Type *>(lp_enum->next());
            lp_queue = reinterpret_cast<void *>(lp_info->iv_id.l);
            ms_ldone_key_dtor_lock(lp_queue, false);
        } else
            lv_done = true;
        delete lp_enum;
        gv_ms_ldone_map.unlock();
    } while (!lv_done);
}

//
// Purpose: getenv (bool)
//
void ms_getenv_bool(const char *pp_key, bool *pp_val) {
    const char *lp_p = ms_getenv_str(pp_key);
    if (lp_p != NULL)
        *pp_val = atoi(lp_p);
}

//
// Purpose: getenv (int)
//
void ms_getenv_int(const char *pp_key, int *pp_val) {
    const char *lp_p = ms_getenv_str(pp_key);
    if (lp_p != NULL)
        *pp_val = atoi(lp_p);
}

//
// Purpose: getenv (long long)
//
void ms_getenv_longlong(const char *pp_key, long long *pp_val) {
    const char *lp_p = ms_getenv_str(pp_key);
    if (lp_p != NULL)
        *pp_val = atoll(lp_p);
}

//
// Purpose: getenv (either regular env or properties file)
//
const char *ms_getenv_str(const char *pp_key) {
    if (!gv_ms_env_loaded) {
        if (!gv_ms_env.load(gp_config_arg_file)) { // try to load arg config
            if (!gv_ms_env.load(gp_config_file)) { // try to load default config
                char *lp_var = getenv(gp_ms_env_sq_var);
                if (lp_var != NULL) {
                    SB_Buf_Lline lv_file;
                    sprintf(&lv_file, "%s/%s", lp_var, gp_config_file);
                    gv_ms_env.load(&lv_file);
                }
            }
        }

        msg_init_env_load();
        gv_ms_env_loaded = true;
    }
    const char *lp_value = gv_ms_env.get(pp_key);
    if (lp_value == NULL)
        lp_value = getenv(pp_key);
    return lp_value;
}

//
// Purpose: mark recv-q items with proc-death
//
#ifdef SQ_PHANDLE_VERIFIER
void ms_recv_q_proc_death(int            pv_nid,
                          int            pv_pid,
                          SB_Verif_Type  pv_verif,
                          bool           pv_use_stream,
                          void          *pp_stream) {
#else
void ms_recv_q_proc_death(int   pv_nid,
                          int   pv_pid,
                          bool  pv_use_stream,
                          void *pp_stream) {
#endif
    const char *WHERE = "ms_recv_q_proc_death";
    MS_Md_Type *lp_md;

    if (gv_ms_trace)
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(WHERE, "p-id=%d/%d/" PFVY "\n",
                           pv_nid, pv_pid, pv_verif);
#else
        trace_where_printf(WHERE, "nid=%d, pid=%d\n",
                           pv_nid, pv_pid);
#endif
    gv_ms_recv_q.lock();
    lp_md = reinterpret_cast<MS_Md_Type *>(gv_ms_recv_q.head());
    while (lp_md != NULL) {
#ifdef SQ_PHANDLE_VERIFIER
        ms_recv_q_proc_death_md(lp_md,
                                pv_nid,
                                pv_pid,
                                pv_verif,
                                pv_use_stream,
                                pp_stream);
#else
        ms_recv_q_proc_death_md(lp_md,
                                pv_nid,
                                pv_pid,
                                pv_use_stream,
                                pp_stream);
#endif
        lp_md = reinterpret_cast<MS_Md_Type *>(lp_md->iv_link.ip_next);
    }
    gv_ms_recv_q.unlock();
    gv_ms_lim_q.lock();
    lp_md = reinterpret_cast<MS_Md_Type *>(gv_ms_lim_q.head());
    while (lp_md != NULL) {
#ifdef SQ_PHANDLE_VERIFIER
        ms_recv_q_proc_death_md(lp_md,
                                pv_nid,
                                pv_pid,
                                pv_verif,
                                pv_use_stream,
                                pp_stream);
#else
        ms_recv_q_proc_death_md(lp_md,
                                pv_nid,
                                pv_pid,
                                pv_use_stream,
                                pp_stream);
#endif
        lp_md = reinterpret_cast<MS_Md_Type *>(lp_md->iv_link.ip_next);
    }
    gv_ms_lim_q.unlock();
}

#ifdef SQ_PHANDLE_VERIFIER
void ms_recv_q_proc_death_md(MS_Md_Type    *pp_md,
                             int            pv_nid,
                             int            pv_pid,
                             SB_Verif_Type  pv_verif,
                             bool           pv_use_stream,
                             void          *pp_stream) {
#else
void ms_recv_q_proc_death_md(MS_Md_Type *pp_md,
                             int         pv_nid,
                             int         pv_pid,
                             bool        pv_use_stream,
                             void       *pp_stream) {
#endif
    const char *WHERE = "ms_recv_q_proc_death_md";

    if (!pp_md->out.iv_mon_msg) {
#ifdef SQ_PHANDLE_VERIFIER
        if ((pp_md->out.iv_nid == pv_nid) &&
            (pp_md->out.iv_pid == pv_pid) &&
            (pp_md->out.iv_verif == pv_verif)) {
#else
        if ((pp_md->out.iv_nid == pv_nid) && (pp_md->out.iv_pid == pv_pid)) {
#endif
            if (!pv_use_stream ||
                (pv_use_stream && (pp_stream == pp_md->ip_stream))) {
                pp_md->out.iv_opts |= SB_Trans::MS_OPTS_PROCDEAD;
                if (gv_ms_trace)
#ifdef SQ_PHANDLE_VERIFIER
                    trace_where_printf(WHERE, "msgid=%d, p-id=%d/%d/" PFVY "\n",
                                       pp_md->iv_link.iv_id.i, pv_nid, pv_pid, pv_verif);
#else
                    trace_where_printf(WHERE, "msgid=%d, nid=%d, pid=%d\n",
                                       pp_md->iv_link.iv_id.i, pv_nid, pv_pid);
#endif
            }
        }
    }
}

//
// Purpose: shutdown
//
void ms_shutdown() {
    ms_ldone_shutdown();
    sb_timer_shutdown();
#ifdef SB_THREAD_LOCK_STATS
    SB_Buf_Lline lv_cmdline;
    char *lp_s = SB_util_get_cmdline(0,
                                     false, // args
                                     &lv_cmdline,
                                     lv_cmdline.size());
    if (lp_s == NULL)
        lp_s = const_cast<char *>("<unknown>");
    SB_Buf_Line la_where;
    sprintf(la_where, "pid=%d, cmd=%s", getpid(), lp_s);
    gv_sb_lock_stats.print(la_where);
#endif
    ms_fifo_fd_restore();
}


void ms_trace_msg_ts(char *pp_str, struct timeval *pp_tv) {
    int lv_hr;
    int lv_min;
    int lv_sec;

    lv_sec = static_cast<int>(pp_tv->tv_sec) % 86400;
    lv_hr = lv_sec / 3600;
    lv_sec %= 3600;
    lv_min = lv_sec / 60;
    lv_sec = lv_sec % 60;
    sprintf(pp_str, "%02d:%02d:%02d.%06ld",
            lv_hr, lv_min, lv_sec, pp_tv->tv_usec);
}



SB_Export SB_Phandle_Type *msg_get_phandle(char *pp_pname) {
   return msg_get_phandle(pp_pname, NULL);
}

SB_Export SB_Phandle_Type *msg_get_phandle(char *pp_pname, int *pp_fserr) {
    const char      *WHERE = "msg_get_phandle";
    char             la_pname[MS_MON_MAX_PROCESS_NAME+1];
    SB_Phandle_Type *lp_phandle;
    int              lv_fserr;
    int              lv_oid;
    SB_Phandle_Type  lv_phandle;
    SB_API_CTR      (lv_zctr, MSG_GET_PHANDLE);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER pname=%s, fserr=%p\n",
                           pp_pname, pfp(pp_fserr));

    SB_util_get_case_insensitive_name(pp_pname, la_pname);

    lp_phandle =
      static_cast<SB_Phandle_Type *>(gv_ms_phandle_map.getv(la_pname));
    if (lp_phandle == NULL) {
        lv_fserr = msg_mon_open_process(la_pname, &lv_phandle, &lv_oid);
        if (lv_fserr == XZFIL_ERR_OK) {
            lv_fserr = msg_set_phandle(la_pname, &lv_phandle);
            SB_util_assert_ieq(lv_fserr, XZFIL_ERR_OK);
            lp_phandle =
              static_cast<SB_Phandle_Type *>(gv_ms_phandle_map.getv(la_pname));
            SB_util_assert_pne(lp_phandle, NULL);
        } else
            lp_phandle = NULL;
    } else
        lv_fserr = XZFIL_ERR_OK;

    if (pp_fserr != NULL)
        *pp_fserr = lv_fserr;

    if (gv_ms_trace_params) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, lp_phandle);
        trace_where_printf(WHERE,
                           "EXIT phandle=%s, fserr=%d\n",
                           la_phandle, lv_fserr);
    }
    return lp_phandle;
}

SB_Export SB_Phandle_Type *msg_get_phandle_no_open(char *pp_pname) {
    const char      *WHERE = "msg_get_phandle_no_open";
    char             la_pname[MS_MON_MAX_PROCESS_NAME+1];
    SB_Phandle_Type *lp_phandle;
    SB_API_CTR      (lv_zctr, MSG_GET_PHANDLE_NO_OPEN);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER pname=%s\n", pp_pname);

    SB_util_get_case_insensitive_name(pp_pname, la_pname);

    lp_phandle =
      static_cast<SB_Phandle_Type *>(gv_ms_phandle_map.getv(la_pname));
    if (gv_ms_trace)
        trace_where_printf(WHERE, "PHANDLES-inuse-count=%d\n",
                           gv_ms_phandle_map.size());
    if (gv_ms_trace_params) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, lp_phandle);
        trace_where_printf(WHERE,
                           "EXIT phandle=%s\n", la_phandle);
    }
    return lp_phandle;
}

SB_Export void msg_getenv_bool(const char *pp_key, bool *pp_val) {
    if (gv_ms_inited)
        ms_getenv_bool(pp_key, pp_val);
}

SB_Export void msg_getenv_int(const char *pp_key, int *pp_val) {
    if (gv_ms_inited)
        ms_getenv_int(pp_key, pp_val);
}

SB_Export const char *msg_getenv_str(const char *pp_key) {
    if (gv_ms_inited)
        return ms_getenv_str(pp_key);
    else
        return NULL;
}

//
// Purpose: initialize msg module
//
SB_Export int msg_init(int *pp_argc, char ***pppp_argv)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, MSG_INIT);
    SB_UTRACE_API_TEST();
    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_INIT, 0);
    return msg_init_com(pp_argc,
                        pppp_argv,
                        true,
                        false,
                        false,
                        NULL,
                        true);
}

//
// Purpose: initialize msg module (attach)
//
SB_Export int msg_init_attach(int    *pp_argc,
                              char ***pppp_argv,
                              int     pv_forkexec,
                              char   *pp_name)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, MSG_INIT_ATTACH);
    SB_UTRACE_API_TEST();
    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_INIT_ATTACH, 0);
    return msg_init_com(pp_argc,
                        pppp_argv,
                        true,
                        true,
                        pv_forkexec,
                        pp_name,
                        true);
}

//
// Purpose: initialize msg module (attach)
//
SB_Export int msg_init_attach_no_msg(int    *pp_argc,
                                     char ***pppp_argv,
                                     int     pv_forkexec,
                                     char   *pp_name)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, MSG_INIT_ATTACH);
    SB_UTRACE_API_TEST();
    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MSG_INIT_ATTACH, 0);
    return msg_init_com(pp_argc,
                        pppp_argv,
                        true,
                        true,
                        pv_forkexec,
                        pp_name,
                        false);
}

//
// Purpose: initialize msg module
//
int msg_init_com(int    *pp_argc,
                 char ***pppp_argv,
                 int     pv_mpi_init,
                 bool    pv_attach,
                 bool    pv_forkexec,
                 char   *pp_name,
                 bool    pv_stderr)
SB_THROWS_FATAL {
    const char        *WHERE = "msg_init";
    struct sigaction   lv_act;
    SB_Buf_Lline       lv_cmdline;
    int                lv_err;
    SB_Buf_Lline       lv_line;
#ifdef USE_SB_SP_LOG
#if 0 // permanently disabled
    int                lv_status;
#endif
#endif

    setvbuf(stdout, NULL, _IOLBF, 0);

    // TODO: remove when evlog gone
    lv_act.sa_handler = SIG_IGN;
    sigemptyset(&lv_act.sa_mask);
    lv_act.sa_flags = 0;
    lv_err = sigaction(SIGPIPE, &lv_act, NULL);
    SB_util_assert_ieq(lv_err, 0);

    gv_ms_su_pthread_self = pthread_self();
    SB_util_get_cmdline(0,
                        false, // args
                        &lv_cmdline,
                        lv_cmdline.size());
    ga_ms_su_prog[SB_MAX_PROG-1] = '\0';
    strncpy(ga_ms_su_prog, basename(&lv_cmdline), SB_MAX_PROG-1);
    SB_thread_main();

    pctl_init();

    if (pv_attach)
        ms_fifo_fd_save();

    pv_forkexec = pv_forkexec; // touch
    if (gv_ms_inited && !gv_ms_init_test)
        return ms_err_rtn_msg(WHERE, "msg-init: already initialized",
                              XZFIL_ERR_INVALIDSTATE);


    // do this here, so we can trace
    msg_init_trace_com(WHERE, *pp_argc, *pppp_argv);

    // if we're supposed to attach, but we're started by shell, don't attach
    if (pv_attach) {
#ifdef SQ_PHANDLE_VERIFIER
        if (*pp_argc >= 10) {
            // "SQMON1.1" <pnid> <nid> <pid> <pname> <port> <ptype> <zid> <verif> "SPARE"
            //     [1]     [2]    [3]   [4]    [5]    [6]    [7]     [8]    [9]     [10]
            if ((!strcmp((*pppp_argv)[1], "SQMON1.1")) &&
                ms_is_mon_number((*pppp_argv)[2]) && // pnid
                ms_is_mon_number((*pppp_argv)[3]) && // nid
                ms_is_mon_number((*pppp_argv)[4]) && // pid
                ms_is_mon_number((*pppp_argv)[7]) && // ptype
                ms_is_mon_number((*pppp_argv)[8]) && // zid
                (strlen((*pppp_argv)[5]) >= 2) &&    // pname
                ((*pppp_argv)[5][0] == '$')) {
                pv_forkexec = false;
                pv_attach = false;
            }
        }
#else
        if (*pp_argc >= 9) {
            // "SQMON1.0" <pnid> <nid> <pid> <pname> <port> <ptype> <zid> "SPARE"
            //     [1]     [2]    [3]   [4]    [5]    [6]    [7]     [8]    [9]
            if ((!strcmp((*pppp_argv)[1], "SQMON1.0")) &&
                ms_is_mon_number((*pppp_argv)[2]) && // pnid
                ms_is_mon_number((*pppp_argv)[3]) && // nid
                ms_is_mon_number((*pppp_argv)[4]) && // pid
                ms_is_mon_number((*pppp_argv)[7]) && // ptype
                ms_is_mon_number((*pppp_argv)[8]) && // zid
                (strlen((*pppp_argv)[5]) >= 2) &&    // pname
                ((*pppp_argv)[5][0] == '$')) {
                pv_forkexec = false;
                pv_attach = false;
            }
        }
#endif
    }

    if (gv_ms_trace_enable) {
        SB_Buf_Line la_line;
        sprintf(la_line, "argc=%d. argv=", *pp_argc);
        msg_trace_args(la_line, *pppp_argv, *pp_argc, sizeof(la_line));
        trace_where_printf(WHERE, "%s\n", la_line);

        ms_gather_info(WHERE);
    }

    if (gv_ms_trace_enable)
        trace_where_printf(WHERE, "TCP set, MS_ASSERT_CHK=%d\n",
                           gv_ms_assert_chk);

    msg_mon_init();

    if (pv_mpi_init) {
        if (!pv_attach) {
            int lv_fserr = msg_mon_init_process_args(WHERE, pp_argc, pppp_argv);
            if (lv_fserr != XZFIL_ERR_OK)
                return ms_err_rtn_msg_fatal(WHERE, "msg-init: process args failed",
                                            static_cast<short>(lv_fserr), true);
        }
#ifdef USE_EVENT_REG
        // automatically register for main thread
        if (getpid() == gettid()) {
            proc_event_register(LREQ);
            proc_event_register(LDONE);
            // set LREQ.
            // This is in case something was received before XWAIT called.
            // The application may see spurious LREQ.
            gv_ms_event_mgr.get_mgr(NULL)->set_event(LREQ, NULL);
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "setting LREQ\n");
        }
#else
        // set LREQ.
        // This is in case something was received before XWAIT called.
        // The application may see spurious LREQ.
        gv_ms_event_mgr.get_mgr(NULL)->set_event(LREQ, NULL);
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "setting LREQ\n");
#endif
        if (pv_attach) {
            int lv_fserr = msg_mon_init_attach(WHERE, pp_name);
            if (lv_fserr != XZFIL_ERR_OK)
                return ms_err_rtn_msg_fatal(WHERE, "msg-init: init-attach failed",
                                            static_cast<short>(lv_fserr),
                                            pv_stderr);
        }
    } else
        gv_ms_init_test = true;


    // setup for start event (pid/etc set)
    SB_util_get_cmdline(0,
                        true, // args
                        &lv_cmdline,
                        lv_cmdline.size());

#ifdef SQ_PHANDLE_VERIFIER
    snprintf(&lv_line, sizeof(lv_line), "msg_init - p-id=%d/%d" PFVY ", tid=%ld, pname=%s, cmdline=%s, %s\n",
#else
    snprintf(&lv_line, sizeof(lv_line), "msg_init - p-id=%d/%d, tid=%ld, pname=%s, cmdline=%s, %s\n",
#endif
            gv_ms_su_nid,
            gv_ms_su_pid,
#ifdef SQ_PHANDLE_VERIFIER
            gv_ms_su_verif,
#endif
            SB_Thread::Sthr::self_id(),
            ga_ms_su_pname,
            &lv_cmdline,
            ms_seabed_vers());

    // setup timer module
    sb_timer_init();

    gv_ms_inited = true;
    gv_ms_calls_ok = true;
    gv_ms_attach = pv_attach;

#ifdef USE_SB_SP_LOG
#if 0 // permanently disabled
    // issue start event (pid/etc set)
    lv_status = SB_log_write_str(SQEVL_SEABED, // USE_SB_SP_LOG
                                 SB_EVENT_ID,
                                 SQ_LOG_SEAQUEST,
                                 SQ_LOG_INFO,
                                 &lv_line);
    CHK_STATUSIGNORE(lv_status);
#endif
#endif

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT OK\n");
    return ms_err_mpi_rtn(WHERE, MPI_SUCCESS);
}

//
// Purpose: initialize environment for msg module
//
void msg_init_env(int pv_argc, char **ppp_argv) {
    char        la_host[200];
    char        la_unique[200];
    const char *lp_p;

    if (gv_ms_pre_inited)
        return;
    gv_ms_pre_inited = true;
    mallopt(M_ARENA_MAX, 1);

    // initialize trace subsystem
    int lv_msenv = -1;
    for (int lv_arg = 0; lv_arg < pv_argc; lv_arg++) {
        char *lp_arg = ppp_argv[lv_arg];
        if (strcmp(lp_arg, "-msenv") == 0)
            lv_msenv = lv_arg + 1;
        else if (lv_arg == lv_msenv) {
            gp_config_arg_file = new char[strlen(lp_arg) + 1];
            strcpy(gp_config_arg_file, lp_arg);
        }
    }

    ms_getenv_bool(gp_ms_env_assert_chk, &gv_ms_assert_chk);
    ms_getenv_bool(gp_ms_env_assert_chk_send, &gv_ms_assert_chk_send);
    ms_getenv_bool(gp_ms_env_assert_error, &gv_ms_assert_error);
    ms_getenv_bool(gp_ms_env_conn_reuse, &gv_ms_conn_reuse);
    ms_getenv_int(gp_ms_env_disc_sem, &gv_ms_disc_sem);
    ms_getenv_bool(gp_ms_env_disc_sem_rob, &gv_ms_disc_sem_rob);
    ms_getenv_bool(gp_ms_env_disc_sem_stats, &gv_ms_disc_sem_stats);
    ms_getenv_int(gp_ms_env_conn_idle_timeout, &gv_ms_conn_idle_timeout);
    ms_getenv_bool(gp_ms_env_shutdown_fast, &gv_ms_shutdown_fast);
    gv_ms_trans_sock = true;
    ms_getenv_bool(gp_ms_env_msg_timestamp, &gv_ms_msg_timestamp);

    // process file vars regardless of enable
    lp_p = ms_getenv_str(gp_ms_env_trace_file_dir);
    if (lp_p != NULL) {
        delete [] gp_ms_trace_file_dir;
        gp_ms_trace_file_dir = new char[strlen(lp_p) + 1];
        strcpy(gp_ms_trace_file_dir, lp_p);
    }
    ms_getenv_longlong(gp_ms_env_trace_file_maxsize,
                       &gv_ms_trace_file_maxsize);
    ms_getenv_bool(gp_ms_env_trace_file_nolock, &gv_ms_trace_file_nolock);
    ms_getenv_int(gp_ms_env_trace_file_unique, &gv_ms_trace_file_unique);
    lp_p = ms_getenv_str(gp_ms_env_trace_file);
    if (lp_p != NULL) {
        delete [] gp_ms_trace_file;
        if (gv_ms_trace_file_unique < 0) {
            gethostname(la_host, sizeof(la_host));
            sprintf(la_unique, "%s.%s.", lp_p, la_host);
            lp_p = la_unique;
        }
        if (gp_ms_trace_file_dir == NULL) {
            gp_ms_trace_file = new char[strlen(lp_p) + 1];
            strcpy(gp_ms_trace_file, lp_p);
        } else {
            gp_ms_trace_file =
              new char[strlen(gp_ms_trace_file_dir) + strlen(lp_p) + 2];
            sprintf(gp_ms_trace_file, "%s/%s", gp_ms_trace_file_dir, lp_p);
        }
    }
    ms_getenv_bool(gp_ms_env_trace_file_delta, &gv_ms_trace_file_delta);
    ms_getenv_int(gp_ms_env_trace_file_fb, &gv_ms_trace_file_fb);
    if (gv_ms_trace_file_fb > 0)
        gv_ms_trace_file_maxsize = 0; // turn off maxsize!
    ms_getenv_int(gp_ms_env_trace_file_inmem, &gv_ms_trace_file_inmem);
    ms_getenv_bool(gp_ms_env_trace_file_signal, &gv_ms_trace_file_signal);
    if (gv_ms_trace_file_signal)
        ms_trace_signal_init();
    lp_p = ms_getenv_str(gp_ms_env_trace_prefix);
    if (lp_p != NULL) {
        delete [] gp_ms_trace_prefix;
        gp_ms_trace_prefix = new char[strlen(lp_p) + 1];
        strcpy(gp_ms_trace_prefix, lp_p);
    }

    ms_getenv_bool(gp_ms_env_trace_enable, &gv_ms_trace_enable);
    if (gv_ms_trace_enable) {
        ms_getenv_bool(gp_ms_env_trace, &gv_ms_trace);
        ms_getenv_bool(gp_ms_env_trace_abandon, &gv_ms_trace_abandon);
        ms_getenv_bool(gp_ms_env_trace_alloc, &gv_ms_trace_alloc);
        ms_buf_trace_change();
        ms_getenv_bool(gp_ms_env_trace_data, &gv_ms_trace_data);
        ms_getenv_int(gp_ms_env_trace_data_max, &gv_ms_trace_data_max);
        ms_getenv_bool(gp_ms_env_trace_detail, &gv_ms_trace_detail);
        ms_getenv_bool(gp_ms_env_trace_dialect, &gv_ms_trace_dialect);
        ms_getenv_bool(gp_ms_env_trace_environ, &gv_ms_trace_environ);
        ms_getenv_bool(gp_ms_env_trace_errors, &gv_ms_trace_errors);
        ms_getenv_bool(gp_ms_env_trace_events, &gv_ms_trace_events);
        SB_Ms_Tl_Event_Mgr::set_trace_events(gv_ms_trace_events);
        ms_getenv_bool(gp_ms_env_trace_evlog, &gv_ms_trace_evlog);
        ms_getenv_bool(gp_ms_env_trace_ic, &gv_ms_trace_ic);
        ms_getenv_bool(gp_ms_env_trace_locio, &gv_ms_trace_locio);
        ms_getenv_bool(gp_ms_env_trace_md, &gv_ms_trace_md);
        ms_getenv_bool(gp_ms_env_trace_mon, &gv_ms_trace_mon);
        ms_getenv_bool(gp_ms_env_trace_name, &gv_ms_trace_name);
        ms_getenv_bool(gp_ms_env_trace_params, &gv_ms_trace_params);
        ms_getenv_bool(gp_ms_env_trace_pthread, &gv_sb_trace_pthread);
        ms_getenv_bool(gp_ms_env_trace_qalloc, &gv_ms_trace_qalloc);
        ms_getenv_bool(gp_ms_env_trace_ref, &gv_ms_trace_ref);
        ms_getenv_bool(gp_ms_env_trace_sm, &gv_ms_trace_sm);
        ms_getenv_bool(gp_ms_env_trace_sock, &gv_ms_trace_sock);
        ms_getenv_bool(gp_ms_env_trace_stats, &gv_ms_trace_stats);
        ms_getenv_bool(gp_ms_env_trace_thread, &gv_sb_trace_thread);
        ms_getenv_bool(gp_ms_env_trace_timer, &gv_ms_trace_timer);
        ms_getenv_bool(gp_ms_env_trace_timermap, &gv_ms_trace_timermap);
        ms_getenv_bool(gp_ms_env_trace_trans, &gv_ms_trace_trans);
        ms_getenv_bool(gp_ms_env_trace_verbose, &gv_ms_trace_verbose);
        ms_getenv_bool(gp_ms_env_trace_wait, &gv_ms_trace_wait);
        ms_getenv_int(gp_ms_env_trace_xx, &gv_ms_trace_xx);
        if (gv_ms_trace || gv_ms_trace_params)
            gv_ms_trace_errors = true;
    }
}

void msg_init_env_load() {
    char la_exe[PATH_MAX];
    bool lv_exe;

    // put the variables into env so others can pick them up
    // pass1 - setenv for non-exe-props
    lv_exe = false;
    SB_Smap_Enum lv_enum1(&gv_ms_env);
    while (lv_enum1.more()) {
        char *lp_key = lv_enum1.next();
        const char *lp_value = gv_ms_env.get(lp_key);
        setenv(lp_key, lp_value, 1);
    }

    // pass2 - setenv for exe-props
    SB_Smap_Enum lv_enum2(&gv_ms_env);
    SB_Props lv_env(true);
    while (lv_enum2.more()) {
        char *lp_key = lv_enum2.next();
        const char *lp_value = gv_ms_env.get(lp_key);
        // if key SQ_PROPS_<exe> matches <exe>, load that prop
        if (memcmp(lp_key, "SQ_PROPS_", 9) == 0) {
            if (!lv_exe) {
                lv_exe = true;
                SB_util_get_exe(la_exe, sizeof(la_exe), true);
            }
            char *lp_key_exe = &lp_key[9];
            if (strcasecmp(lp_key_exe, la_exe) == 0)
                msg_init_env_load_exe(lp_value, &lv_env);
            else if (isdigit(la_exe[0]) ||
                     ((la_exe[0] >= 'a') && (la_exe[0] <= 'f'))) {
                // check for clearcase view
                // filenames look like: 800004064c0e6923tdm_arkcmp
                int lv_len = static_cast<int>(strlen(lp_key_exe));
                int lv_off = static_cast<int>(strlen(la_exe)) - lv_len;
                if (strcasecmp(lp_key_exe, &la_exe[lv_off]) == 0)
                    msg_init_env_load_exe(lp_value, &lv_env);
            }
        }
    }
    // copy exe-props into gv_ms_env/setenv
    SB_Smap_Enum lv_enum3(&lv_env);
    while (lv_enum3.more()) {
        char *lp_key = lv_enum3.next();
        const char *lp_value = lv_env.get(lp_key);
        gv_ms_env.put(lp_key, lp_value);
        setenv(lp_key, lp_value, 1);
    }

}

void msg_init_env_load_exe(const char *pp_exe, SB_Props *pp_env) {
    SB_Props lv_exe_env(true);

    if (!lv_exe_env.load(pp_exe)) {
        char *lp_var = getenv(gp_ms_env_sq_var);
        if (lp_var != NULL) {
            SB_Buf_Lline lv_file;
            sprintf(&lv_file, "%s/%s", lp_var, pp_exe);
            lv_exe_env.load(&lv_file);
        }
    }
    SB_Smap_Enum lv_enum(&lv_exe_env);
    while (lv_enum.more()) {
        char *lp_key = lv_enum.next();
        const char *lp_value = lv_exe_env.get(lp_key);
        pp_env->put(lp_key, lp_value);
    }
}

SB_Export void msg_init_trace() {
    SB_API_CTR (lv_zctr, MSG_INIT_TRACE);
    msg_init_trace_com("msg_init_trace", 0, NULL);
}

void msg_init_trace_com(const char *pp_where, int pv_argc, char **ppp_argv) {
    char         **lpp_env;
    SB_Buf_Lline   lv_line;

    msg_init_env(pv_argc, ppp_argv);
    trace_set_assert_no_trace(gv_ms_assert_error);
    if (gv_ms_trace_enable) {
        trace_set_delta(gv_ms_trace_file_delta);
        if (gv_ms_trace_file_nolock)
            trace_set_lock(!gv_ms_trace_file_nolock);
        trace_init2(gp_ms_trace_file,
                    gv_ms_trace_file_unique,
                    gp_ms_trace_prefix,
                    false,
                    gv_ms_trace_file_maxsize);
    }
    if (gv_ms_trace_file_fb > 0)
        trace_set_mem(gv_ms_trace_file_fb);
    if (gv_ms_trace_file_inmem > 0)
        trace_set_inmem(gv_ms_trace_file_inmem);

    // trace start
    sprintf(&lv_line, "SEABED module version %s\n",
            ms_seabed_vers());
    if (gv_ms_trace_enable) {
        SB_Buf_Line la_line;
        trace_printf("%s\n", libsbms_vers2_str());
        trace_where_printf(pp_where, &lv_line);
        SB_util_get_cmdline(0,
                            true, // args
                            la_line,
                            sizeof(la_line));
        trace_where_printf(pp_where, "cmdline=%s\n", la_line);
    }
    if (gv_ms_trace_environ) {
        lpp_env = environ;
        while (*lpp_env != NULL) {
            trace_where_printf(pp_where, "env=%s\n", *lpp_env);
            lpp_env++;
        }
    }
}

SB_Export void  msg_test_disable_wait(int pv_disable_wait) {
    gv_ms_disable_wait = pv_disable_wait;
    gv_ms_disable_wait = gv_ms_disable_wait; // touch
}

SB_Export int msg_test_enable_client_only(void) {
    int lv_fserr;

    if (gv_ms_inited)
        lv_fserr = XZFIL_ERR_INVALIDSTATE;
    else {
        lv_fserr = XZFIL_ERR_OK;
        gv_ms_client_only = true;
    }
    return lv_fserr;
}




SB_Export void msg_test_set_md_count(int pv_count) {
    SB_Trans::Msg_Mgr::test_set_md_count(pv_count);
}

//
// Purpose: emulate MSG_ABANDON_
//
SB_Export short BMSG_ABANDON_(int pv_msgid) {
    const char *WHERE = "BMSG_ABANDON_";
    short       lv_fserr;
    MS_Md_Type *lp_md;
    SB_API_CTR (lv_zctr, BMSG_ABANDON_);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_MSG_ABANDON, pv_msgid);
    lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER msgid=%d\n", pv_msgid);
    else if (gv_ms_trace || gv_ms_trace_abandon)
        trace_where_printf(WHERE, "msgid=%d\n", pv_msgid);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, NULL); // don't mess up where (ABANDON)
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    if (gv_ms_trace || gv_ms_trace_abandon)
        trace_where_printf(WHERE, "msg=%p\n", pfp(lp_md));

    //
    // Mark abandon.
    // If it's already in completion-queue, simply remove it from queue;
    // otherwise, send abandon to remote
    //
    lp_md->iv_abandoned = true;
    SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
    if ((lp_stream != NULL) && lp_stream->remove_comp_q(lp_md)) {
        // found on queue, simply remove it
        if (gv_ms_trace || gv_ms_trace_abandon)
            trace_where_printf(WHERE, "msgid=%d found on completion queue and removed\n",
                               pv_msgid);
        lp_md->ip_mgr->change_replies_done(-1, LDONE);
        SB_Trans::Msg_Mgr::put_md_link(lp_md, "abandon comp-q");
    } else if (lp_md->iv_reply_done) {
        if (gv_ms_trace || gv_ms_trace_abandon)
            trace_where_printf(WHERE, "msgid=%d reply already done\n", pv_msgid);
        lp_md->ip_mgr->change_replies_done(-1, LDONE);
        if (lp_md->out.iv_opts & (SB_Trans::MS_OPTS_LDONE | SB_Trans::MS_OPTS_FSDONE))
            SB_Trans::Trans_Stream::remove_comp_q_static(lp_md);
        SB_Trans::Msg_Mgr::put_md_link(lp_md, "abandon no-q");
    } else if (lp_stream != NULL) {
        // send abandon to remote
        MS_Md_Type *lp_can_md;
        SB_Trans::Msg_Mgr::get_md(&lp_can_md, // BMSG_ABANDON_
                                  lp_stream,
                                  gv_ms_event_mgr.get_mgr(NULL),
                                  true,       // send
                                  NULL,       // fserr
                                  "abandon (BMSG_ABANDON_)",
                                  MD_STATE_MSG_LOW_ABANDON);
        SB_util_assert_pne(lp_can_md, NULL); // TODO: can't get md
        lv_fserr = lp_stream->exec_abandon(lp_can_md,
                                           lp_can_md->iv_link.iv_id.i, // reqid
                                           lp_md->iv_link.iv_id.i);    // can_reqid
        if (lv_fserr == XZFIL_ERR_OK)
            lp_stream->wait_rep_done(lp_can_md);
        else
            lp_stream->error_sync();
        SB_Trans::Msg_Mgr::put_md_link(lp_can_md, "abandon md");

        // it is possible that we missed the check above
        if (lp_stream->remove_comp_q(lp_md)) {
            // found on queue, simply remove it
            if (gv_ms_trace || gv_ms_trace_abandon)
                trace_where_printf(WHERE, "msgid=%d found on completion queue after abandon and removed\n",
                                   pv_msgid);
            lp_md->ip_mgr->change_replies_done(-1, LDONE);
            SB_Trans::Msg_Mgr::put_md_link(lp_md, "abandon comp-q");
        }
    } else {
        if (gv_ms_trace || gv_ms_trace_abandon)
            trace_where_printf(WHERE, "msgid=%d stream is NULL\n", pv_msgid);
        lp_md->ip_mgr->change_replies_done(-1, LDONE);
        SB_Trans::Msg_Mgr::put_md_link(lp_md, "abandon no-stream");
    }
    if (gv_ms_trace_params || gv_ms_trace_abandon)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: emulate MSG_AWAIT_
//
SB_Export short BMSG_AWAIT_(int pv_msgid, int pv_tov) {
    SB_API_CTR (lv_zctr, BMSG_AWAIT_);
    while (!BMSG_ISDONE_(pv_msgid)) {
        if (XWAIT(LDONE, pv_tov) == 0)
            return 1;
    }
    return 0;
}

//
// Purpose: emulate MSG_BREAK_
//
// do receive (if not already done)
// get results
//
SB_Export short BMSG_BREAK_(int              pv_msgid,
                            short           *pp_results,
                            SB_Phandle_Type *pp_phandle) {
    SB_API_CTR (lv_zctr, BMSG_BREAK_);
    return xmsg_break_com(pv_msgid, pp_results, pp_phandle, NULL, false);
}

//
// Purpose: emulate MSG_BREAK_
//
// do receive (if not already done)
// get results
//
SB_Export void BMSG_BREAK2_(int              pv_msgid,
                            short           *pp_results,
                            SB_Phandle_Type *pp_phandle) {
    SB_API_CTR (lv_zctr, BMSG_BREAK_);
    xmsg_break_com(pv_msgid, pp_results, pp_phandle, NULL, true);
}

//
// Purpose: common MSG_BREAK_
//
// if ppp_md is NOT NULL, then md is returned (not put)
//
short xmsg_break_com(int               pv_msgid,
                     short            *pp_results,
                     SB_Phandle_Type  *pp_phandle,
                     MS_Md_Type      **ppp_md,
                     bool              pv_reply_ctrl) {
    const char         *WHERE = "BMSG_BREAK_";
    MS_Md_Type         *lp_md;
    MS_Result_Type     *lp_results;
    MS_Result_Raw_Type *lp_results_raw;
    long long           lv_delta;
    short               lv_fserr;
    bool                lv_trace_delta;

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_MSG_BREAK, pv_msgid);
    if (gv_ms_trace_params) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, pp_phandle);
        trace_where_printf(WHERE,
                           "ENTER from=%s, msgid=%d, results=%p, phandle=%s\n",
                           ms_od_map_phandle_to_name(pp_phandle), pv_msgid,
                           pfp(pp_results), la_phandle);
    }
    if (ppp_md != NULL)
        *ppp_md = NULL;
    // don't mess up where (yet) (BREAK)
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, NULL);
    if (lp_md == NULL) {
        if (pv_reply_ctrl)
            ms_err_break_err(WHERE, XZFIL_ERR_NOTFOUND, lp_md, pp_results);
        return ms_err_rtn_msg(WHERE, " EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    }
    if (lp_md->iv_break_done) {
        if (pv_reply_ctrl)
            ms_err_break_err(WHERE, XZFIL_ERR_TOOMANY, lp_md, pp_results);
        return ms_err_rtn_msg(WHERE, " EXIT [break already done]", XZFIL_ERR_TOOMANY);
    }
    lp_md->iv_break_done = true;
    lp_results = reinterpret_cast<MS_Result_Type *>(pp_results);
    lp_results_raw = reinterpret_cast<MS_Result_Raw_Type *>(pp_results);
    SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
    lv_trace_delta = false;
    if (lp_stream == NULL) {
        SB_Trans::Trans_Stream::wait_req_done_static(lp_md);
        if (lp_md->out.iv_opts & (SB_Trans::MS_OPTS_LDONE | SB_Trans::MS_OPTS_FSDONE))
            SB_Trans::Trans_Stream::remove_comp_q_static(lp_md);
        else if (gv_ms_trace_params)
            lv_trace_delta = true;
    } else {
        lp_stream->wait_req_done(lp_md);
        if (lp_md->out.iv_opts & (SB_Trans::MS_OPTS_LDONE | SB_Trans::MS_OPTS_FSDONE))
            lp_stream->remove_comp_q(lp_md);
        else if (gv_ms_trace_params)
            lv_trace_delta = true;
    }
    if (gv_ms_msg_timestamp)
        gettimeofday(&lp_md->iv_ts_msg_cli_break, NULL);
    if (lv_trace_delta) {
        gettimeofday(&lp_md->out.iv_comp_q_off_tod, NULL);
        lv_delta = (lp_md->out.iv_comp_q_off_tod.tv_sec * SB_US_PER_SEC +
                    lp_md->out.iv_comp_q_off_tod.tv_usec) -
                   (lp_md->out.iv_comp_q_on_tod.tv_sec * SB_US_PER_SEC +
                    lp_md->out.iv_comp_q_on_tod.tv_usec);
        trace_where_printf(WHERE,
                           "done-to-break, (msgid=%d, md=%p) time=%lld us\n",
                           lp_md->iv_link.iv_id.i, pfp(lp_md),
                           lv_delta);
    }
    lp_md->ip_mgr->change_replies_done(-1, LDONE);
    lv_fserr = lp_md->out.iv_fserr;


    lp_results->rr_ctrlsize = lp_md->out.iv_reply_ctrl_count;
    lp_results->rr_datasize = lp_md->out.iv_reply_data_count;
    lp_results_raw->rrerr = 0;
    if (lv_fserr != XZFIL_ERR_OK)
        ms_fserr_to_results(lv_fserr, lp_results);
    if (lp_md->out.iv_reply_data_count > 0)
        lp_results->rrerr_datareceivedb = 1;
    else
        lp_results->rrerr_datareceivedb = 0;
    if ((lv_fserr != XZFIL_ERR_OK) && pv_reply_ctrl)
        ms_err_break_err(WHERE, lv_fserr, lp_md, pp_results);
    if (lv_fserr == XZFIL_ERR_OK) {
        if (gv_ms_trace_data) {
            trace_where_printf(WHERE,
                               "repC=%p, repCcount=%d\n",
                               pfp(lp_md->out.ip_reply_ctrl),
                               lp_md->out.iv_reply_ctrl_count);
            trace_print_data(lp_md->out.ip_reply_ctrl,
                             lp_md->out.iv_reply_ctrl_count,
                             gv_ms_trace_data_max);
            trace_where_printf(WHERE,
                               "repD=%p, repDcount=%d\n",
                               lp_md->out.ip_reply_data,
                               lp_md->out.iv_reply_data_count);
            trace_print_data(lp_md->out.ip_reply_data,
                             lp_md->out.iv_reply_data_count,
                             gv_ms_trace_data_max);
        }
    }
    // TODO remove next two lines
    if (!gv_ms_recv_q.remove_list(&lp_md->iv_link))
        lp_md->ip_fs_comp_q->remove_list(&lp_md->iv_link);

    if ((lv_fserr != XZFIL_ERR_OK) &&
        (lp_md->out.iv_fserr_hard) &&
        (lp_stream != NULL)) {
        if (lp_stream->error_of_generation(lp_md->out.iv_fserr_generation)) {
            if (gv_ms_trace)
                trace_where_printf(WHERE, "fserr=%d in md, msgid=%d, gen=%d\n",
                                   lv_fserr, pv_msgid,
                                   lp_md->out.iv_fserr_generation);
            // close stream
            msg_mon_close_process_com(pp_phandle, false); // don't free oid
        }
    }

    if (ppp_md == NULL)
        SB_Trans::Msg_Mgr::put_md_link(lp_md, "msg-break");
    else
        *ppp_md = lp_md;

    if (gv_ms_trace_params) {
        if (gv_ms_msg_timestamp) {
            // 123456789012345
            // HH:MM:SS.UUUUUU
            char la_c_link[20];
            char la_s_rcvd[20];
            char la_s_list[20];
            char la_s_reply[20];
            char la_c_rcvd[20];
            char la_c_break[20];
            ms_trace_msg_ts(la_c_link, &lp_md->iv_ts_msg_cli_link);
            ms_trace_msg_ts(la_s_rcvd, &lp_md->iv_ts_msg_srv_rcvd);
            ms_trace_msg_ts(la_s_list, &lp_md->iv_ts_msg_srv_listen);
            ms_trace_msg_ts(la_s_reply, &lp_md->iv_ts_msg_srv_reply);
            ms_trace_msg_ts(la_c_rcvd, &lp_md->iv_ts_msg_cli_rcvd);
            ms_trace_msg_ts(la_c_break, &lp_md->iv_ts_msg_cli_break);
            trace_where_printf(WHERE,
                               "c-link=%s, s-rcvd=%s, s-list=%s, s-reply=%s, c-rcvd=%s, c-break=%s\n",
                               la_c_link,
                               la_s_rcvd,
                               la_s_list,
                               la_s_reply,
                               la_c_rcvd,
                               la_c_break);
        }
        trace_where_printf(WHERE,
                           "EXIT ret=%d, msgid=%d, results.ctrl=%u, results.data=%u\n",
                           lv_fserr,
                           pv_msgid,
                           lp_results->rr_ctrlsize,
                           lp_results->rr_datasize);
    } else if (gv_ms_trace && (lv_fserr != XZFIL_ERR_OK))
        trace_where_printf(WHERE, "EXIT ret=%d, msgid=%d\n",
                           lv_fserr, pv_msgid);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: emulate MSG_GETREQINFO_
//
SB_Export short BMSG_GETREQINFO_(int     pv_item_code,
                                 int     pv_msgid,
                                 int    *pp_item) {
    const char *WHERE = "BMSG_GETREQINFO_";
    MS_Md_Type *lp_md;
    int         lv_nid;
    int         lv_pid;
    int         lv_ptype;
    SB_API_CTR (lv_zctr, BMSG_GETREQINFO_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER item-code=%d, msgid=%d, item=%p\n",
                           pv_item_code, pv_msgid, pfp(pp_item));
    if (!gv_ms_calls_ok)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_item == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [item == NULL]",
                              XZFIL_ERR_BOUNDSERR);

    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, NULL); // don't mess up where
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    switch (pv_item_code) {
    case MSGINFO_NID:
        lv_nid = lp_md->out.iv_nid;
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "item=%d (nid)\n", lv_nid);
        *pp_item = lv_nid;
        break;
    case MSGINFO_PID:
        lv_pid = lp_md->out.iv_pid;
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "item=%d (pid)\n", lv_pid);
        *pp_item = lv_pid;
        break;
    case MSGINFO_PTYPE:
        lv_ptype = lp_md->out.iv_ptype;
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "item=%d (ptype)\n", lv_ptype);
        *pp_item = lv_ptype;
        break;
    default:
        return ms_err_rtn_msg(WHERE, " EXIT [invalid item-code]",
                              XZFIL_ERR_BOUNDSERR);
    }
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=0\n");
    return ms_err_rtn(XZFIL_ERR_OK);
}

//
// Purpose: emulate MSG_HOLD_
//
SB_Export void BMSG_HOLD_(int pv_msgid) {
    const char *WHERE = "BMSG_HOLD_";
    MS_Md_Type *lp_md;
    SB_API_CTR (lv_zctr, BMSG_HOLD_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER msgid=%d\n", pv_msgid);
    // NSK would do a halt
    SB_util_assert_it(gv_ms_calls_ok); // sw fault
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, NULL); // don't mess up where
    if (lp_md == NULL) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT unknown msgid=%d\n", pv_msgid);
        // NSK does a halt
        SB_util_assert_pne(lp_md, NULL); // sw fault
    }
    gv_ms_hold_q.add(&lp_md->iv_link);
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// Purpose: emulate MSG_ISCANCELED_
//
SB_Export short BMSG_ISCANCELED_(int pv_msgid) {
    const char *WHERE = "BMSG_ISCANCELED_";
    short       lv_abandoned;
    MS_Md_Type *lp_md;
    SB_API_CTR (lv_zctr, BMSG_ISCANCELED_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER msgid=%d\n", pv_msgid);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, NULL); // don't mess up where (ISCANCELED)
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    lv_abandoned = lp_md->iv_abandoned;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_abandoned);
    return lv_abandoned;
}

//
// Purpose: emulate MSG_ISDONE_
//
// do receive (if not already done)
//
SB_Export short BMSG_ISDONE_(int pv_msgid) {
    const char *WHERE = "BMSG_ISDONE_";
    short       lv_done;
    MS_Md_Type *lp_md;
    SB_API_CTR (lv_zctr, BMSG_ISDONE_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER msgid=%d\n", pv_msgid);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, NULL); // don't mess up where (ISDONE)
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    lv_done = lp_md->iv_reply_done;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_done);
    return lv_done;
}

//
// Purpose: emulate MSG_LINK_
//
short BMSG_LINK_common(SB_Phandle_Type *pp_phandle,
                       int             *pp_msgid,
                       short           *pp_reqctrl,
                       int              pv_reqctrlsize,
                       short           *pp_replyctrl,
                       int              pv_replyctrlmax,
                       char            *pp_reqdata,
                       int              pv_reqdatasize,
                       char            *pp_replydata,
                       int              pv_replydatamax,
                       long             pv_linkertag,
                       short            pv_pri,
                       short            pv_xmitclass,
                       short            pv_linkopts,
                       bool             pv_nskcheck) {
    const char  *WHERE = "BMSG_LINK_";
    SB_Buf_Line  la_log_buf;
    MS_Md_Type  *lp_md;
    void        *lp_od;
    short        lv_fserr;
    short        lv_lfserr;
    int          lv_msgid;
    int          lv_opts;
    bool         lv_ts;
    SB_API_CTR  (lv_zctr, BMSG_LINK_);

    if (gv_ms_trace_params) {
        trace_where_printf(WHERE, "ENTER to=%s, msgid=%p, reqC=%p, size=%d, repC=%p, max=%d\n",
                           ms_od_map_phandle_to_name(pp_phandle),
                           pfp(pp_msgid),
                           pfp(pp_reqctrl), pv_reqctrlsize,
                           pfp(pp_replyctrl), pv_replyctrlmax);
        trace_where_printf(WHERE,
                           "reqD=%p, size=%d, repD=%p, max=%d, ltag=0x%lx, pri=%d, xcls=%d, lopts=0x%x\n",
                           pp_reqdata, pv_reqdatasize,
                           pp_replydata, pv_replydatamax,
                           pv_linkertag, pv_pri, pv_xmitclass, pv_linkopts);
    }
    if (pp_msgid != NULL)
        *pp_msgid = 0; // mark no need for break
    if (!gv_ms_calls_ok)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_phandle == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [phandle == NULL]",
                              XZFIL_ERR_BOUNDSERR);
    if (pp_msgid == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msgid == NULL]",
                              XZFIL_ERR_BOUNDSERR);
    SB_util_assert_ige(pv_reqctrlsize, 0); // insist ctrl is non-negative
    SB_util_assert_ige(pv_reqdatasize, 0); // insist data is non-negative
    if (pv_nskcheck) {
        if (pp_reqctrl == NULL) {
            sprintf(la_log_buf, "%s: reqC is NULL", WHERE);
            if (gv_ms_trace_errors)
                trace_printf("%s\n", la_log_buf);
            SB_util_fatal(la_log_buf, true);
        }
        if (pv_reqctrlsize & 1) {
            sprintf(la_log_buf, "%s: reqCsize(%d) must be even\n",
                    WHERE, pv_reqctrlsize);
            if (gv_ms_trace_errors)
                trace_printf("%s\n", la_log_buf);
            SB_util_fatal(la_log_buf, true);
        }
        if (pv_reqctrlsize < MSG_MINREQCTRLSIZE) {
            sprintf(la_log_buf, "%s: reqCsize(%d) is too small, needs to be at least %d\n",
                    WHERE, pv_reqctrlsize, MSG_MINREQCTRLSIZE);
            if (gv_ms_trace_errors)
                trace_printf("%s\n", la_log_buf);
            SB_util_fatal(la_log_buf, true);
        }
        if (pv_reqctrlsize > MSG_MAXREQCTRLSIZE) {
            sprintf(la_log_buf, "%s: reqCsize(%d) is too big, must not be bigger than %d\n",
                    WHERE, pv_reqctrlsize, MSG_MAXREQCTRLSIZE);
            if (gv_ms_trace_errors)
                trace_printf("%s\n", la_log_buf);
            SB_util_fatal(la_log_buf, true);
        }
        if (pp_replyctrl == NULL) {
            sprintf(la_log_buf, "%s: repC is NULL\n", WHERE);
            if (gv_ms_trace_errors)
                trace_printf("%s\n", la_log_buf);
            SB_util_fatal(la_log_buf, true);
        }
        if (pv_replyctrlmax & 1) {
            sprintf(la_log_buf, "%s: repCmax(%d) must be even\n",
                    WHERE, pv_replyctrlmax);
            if (gv_ms_trace_errors)
                trace_printf("%s\n", la_log_buf);
            SB_util_fatal(la_log_buf, true);
        }
        if (pv_replyctrlmax < MSG_MINREPLYCTRLSIZE) {
            sprintf(la_log_buf, "%s: repCmax(%d) is too small, needs to be at least %d\n",
                    WHERE, pv_reqctrlsize, MSG_MINREPLYCTRLSIZE);
            if (gv_ms_trace_errors)
                trace_printf("%s\n", la_log_buf);
            SB_util_fatal(la_log_buf, true);
        }
        if (pv_replyctrlmax > MSG_MAXREPLYCTRLSIZE) {
            sprintf(la_log_buf, "%s: repCmax(%d) is too big, must not be bigger than %d\n",
                    WHERE, pv_reqctrlsize, MSG_MAXREPLYCTRLSIZE);
            if (gv_ms_trace_errors)
                trace_printf("%s\n", la_log_buf);
            SB_util_fatal(la_log_buf, true);
        }
    }
    int lv_oid = ms_od_map_phandle_to_oid(pp_phandle);
    if (!ms_od_valid_oid(lv_oid))
        return ms_err_rtn_msg(WHERE, "EXIT [bad oid in phandle]",
                              XZFIL_ERR_NOTOPEN);
    lp_od = ms_od_lock(lv_oid);
    SB_Trans::Stream_Base *lp_stream = ms_od_map_phandle_to_stream(pp_phandle);
    if ((lv_oid > 0) && (lp_stream == NULL)) {
        // there's an open, but no stream
        lv_fserr = static_cast<short>(msg_mon_reopen_process(pp_phandle));
        if (lv_fserr != XZFIL_ERR_OK) {
            ms_od_unlock(lp_od);
            lv_msgid = SB_Trans::Msg_Mgr::get_md(&lp_md,     // BMSG_LINK_
                                                 NULL,
                                                 gv_ms_event_mgr.get_mgr(NULL),
                                                 true,       // send
                                                 &lv_lfserr, // fserr
                                                 WHERE,
                                                 MD_STATE_MSG_REOPEN_FAIL);
            if (lv_msgid < 0)
                return ms_err_rtn_msg(WHERE, "EXIT [msgid < 0]",
                                      lv_lfserr);
            // fill out md enough to complete
            lv_opts = 0;
            lv_ts = (pv_linkopts & XMSG_LINK_FSDONETSQ);
            if (lv_ts)
                lv_opts |= SB_Trans::MS_OPTS_FSDONETS;
            if (pv_linkopts & XMSG_LINK_FSDONEQ)
                lv_opts |= SB_Trans::MS_OPTS_FSDONE;
            else if (pv_linkopts & XMSG_LINK_LDONEQ)
                lv_opts |= SB_Trans::MS_OPTS_LDONE;
            lp_md->out.iv_opts = lv_opts;
            lp_md->iv_tag = pv_linkertag;
            lp_md->ip_comp_q = ms_ldone_get_comp_q();
            lp_md->ip_fs_comp_q = ms_fsdone_get_comp_q(lv_ts);
            lp_md->iv_ss.ip_req_ctrl = pp_reqctrl;
            lp_md->iv_ss.iv_req_ctrl_size = pv_reqctrlsize;
            lp_md->iv_ss.ip_req_data = pp_reqdata;
            lp_md->iv_ss.iv_req_data_size = pv_reqdatasize;
            lp_md->out.ip_reply_ctrl = pp_replyctrl,
            lp_md->out.iv_reply_ctrl_max = pv_replyctrlmax,
            lp_md->out.ip_reply_data = pp_replydata;
            lp_md->out.iv_reply_data_max = pv_replydatamax,
            lp_md->iv_send_done = true;

            SB_Trans::Trans_Stream::finish_reply_static(lp_md,
                                                        lv_fserr,
                                                        true,
                                                        -1,                          // generation
                                                        NULL,                        // req_map
                                                        true,
                                                        msg_mon_is_self(pp_phandle), // self
                                                        &ms_ldone_cbt);              // ms_comp_callback
            lv_fserr = XZFIL_ERR_OK; // even if there's an error, clear it
            *pp_msgid = lv_msgid;
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "EXIT ret=%d, msgid=%d\n",
                                   lv_fserr, lv_msgid);
            return ms_err_rtn(lv_fserr);
        }
        // reacquire stream
        lp_stream = ms_od_map_phandle_to_stream(pp_phandle);
    }
    lv_msgid = SB_Trans::Msg_Mgr::get_md(&lp_md,    // BMSG_LINK_
                                         lp_stream,
                                         gv_ms_event_mgr.get_mgr(NULL),
                                         true,      // send
                                         &lv_fserr, // fserr
                                         WHERE,
                                         MD_STATE_MSG_LINK);
    ms_od_unlock(lp_od); // after get_md - ref added
    if (lv_msgid < 0)
        return ms_err_rtn_msg(WHERE, "EXIT [msgid < 0]", lv_fserr);
    lp_md->ip_comp_q = ms_ldone_get_comp_q();
    lv_ts = (pv_linkopts & XMSG_LINK_FSDONETSQ);
    lp_md->ip_fs_comp_q = ms_fsdone_get_comp_q(lv_ts);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "to=%s, msgid=%d, reqC=%d, reqD=%d\n",
                           ms_od_map_phandle_to_name(pp_phandle),
                           lv_msgid, pv_reqctrlsize, pv_reqdatasize);
    if (gv_ms_trace_data) {
        trace_where_printf(WHERE,
                           "reqC=%p, reqCsize=%d\n",
                           pfp(pp_reqctrl), pv_reqctrlsize);
        trace_print_data(pp_reqctrl, pv_reqctrlsize, gv_ms_trace_data_max);
        trace_where_printf(WHERE,
                           "reqD=%p, reqDsize=%d\n",
                           pp_reqdata, pv_reqdatasize);
        trace_print_data(pp_reqdata, pv_reqdatasize, gv_ms_trace_data_max);
    }
    lv_opts = 0;
    if (pv_linkopts) {
        if (lv_ts)
            lv_opts |= SB_Trans::MS_OPTS_FSDONETS;
        if (pv_linkopts & BMSG_LINK_FSDONEQ)
            lv_opts |= SB_Trans::MS_OPTS_FSDONE;
        else if (pv_linkopts & BMSG_LINK_LDONEQ)
            lv_opts |= SB_Trans::MS_OPTS_LDONE;
        if (pv_linkopts & XMSG_LINK_FSREQ)
            lv_opts |= SB_Trans::MS_OPTS_FSREQ;
        if (pv_linkopts & BMSG_LINK_MSINTERCEPTOR)
            lv_opts |= SB_Trans::MS_OPTS_MSIC;
    }
    lp_md->out.iv_opts = lv_opts;
    lp_md->iv_tag = pv_linkertag;
    lv_fserr = lp_stream->exec_wr(lp_md,
                                  0,                      // src
                                  0,                      // dest
                                  lp_md->iv_link.iv_id.i, // reqid
                                  pv_pri,
                                  pp_reqctrl,
                                  pv_reqctrlsize,
                                  pp_reqdata,
                                  pv_reqdatasize,
                                  pp_replyctrl,
                                  pv_replyctrlmax,
                                  pp_replydata,
                                  pv_replydatamax,
                                  lv_opts);               // opts
    lv_fserr = XZFIL_ERR_OK; // even if there's an error, clear it
    *pp_msgid = lv_msgid;
    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_MSG_LINK, lv_msgid);
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d, msgid=%d\n",
                           lv_fserr, lv_msgid);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: emulate MSG_LINK_
//
SB_Export short BMSG_LINK_(SB_Phandle_Type *pp_phandle,
                           int             *pp_msgid,
                           short           *pp_reqctrl,
                           int              pv_reqctrlsize,
                           short           *pp_replyctrl,
                           int              pv_replyctrlmax,
                           char            *pp_reqdata,
                           int              pv_reqdatasize,
                           char            *pp_replydata,
                           int              pv_replydatamax,
                           long             pv_linkertag,
                           short            pv_pri,
                           short            pv_xmitclass,
                           short            pv_linkopts) {
    return BMSG_LINK_common(pp_phandle,
                            pp_msgid,
                            pp_reqctrl,
                            pv_reqctrlsize,
                            pp_replyctrl,
                            pv_replyctrlmax,
                            pp_reqdata,
                            pv_reqdatasize,
                            pp_replydata,
                            pv_replydatamax,
                            pv_linkertag,
                            pv_pri,
                            pv_xmitclass,
                            pv_linkopts,
                            false);
}

//
// Purpose: emulate MSG_LINK_
//
SB_Export short BMSG_LINK2_(SB_Phandle_Type *pp_phandle,
                            int             *pp_msgid,
                            short           *pp_reqctrl,
                            int              pv_reqctrlsize,
                            short           *pp_replyctrl,
                            int              pv_replyctrlmax,
                            char            *pp_reqdata,
                            int              pv_reqdatasize,
                            char            *pp_replydata,
                            int              pv_replydatamax,
                            long             pv_linkertag,
                            short            pv_pri,
                            short            pv_xmitclass,
                            short            pv_linkopts) {
    return BMSG_LINK_common(pp_phandle,
                            pp_msgid,
                            pp_reqctrl,
                            pv_reqctrlsize,
                            pp_replyctrl,
                            pv_replyctrlmax,
                            pp_reqdata,
                            pv_reqdatasize,
                            pp_replydata,
                            pv_replydatamax,
                            pv_linkertag,
                            pv_pri,
                            pv_xmitclass,
                            pv_linkopts,
                            true); // nsk check
}

//
// Purpose: emulate MSG_LISTEN_
//
static short BMSG_LISTEN_common(short *pp_sre,
                                short  pv_listenopts,
                                long   pv_listenertag,
                                bool   pv_big) {
    const char *WHERE = "BMSG_LISTEN_";
    bool        lv_abandoned = false;
    short       lv_fserr;
    bool        lv_repeat;
    bool        lv_traced = false;
    MS_Md_Type *lp_md = NULL;
    void       *lp_tle = NULL;
    SB_API_CTR (lv_zctr, BMSG_LISTEN_);

    if (gv_ms_trace_params && gv_ms_trace_verbose)
        trace_where_printf(WHERE,
                           "ENTER sre=%p, listenopts=0x%x, listenertag=%ld\n",
                           pfp(pp_sre), pv_listenopts, pv_listenertag);
    if (!gv_ms_calls_ok)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XSRETYPE_NOWORK);
    if (pv_listenopts == 0)
        pv_listenopts = BLISTEN_DEFAULTM;
    else if (pv_listenopts == BLISTEN_TEST_ILIMREQM) {
        if (gv_ms_trace_params && gv_ms_trace_verbose)
            trace_where_printf(WHERE,
                               "ENTER sre=%p, listenopts=0x%x, listenertag=%ld\n",
                               pfp(pp_sre), pv_listenopts, pv_listenertag);
        gv_ms_lim_q.lock();
        lp_md = reinterpret_cast<MS_Md_Type *>(gv_ms_lim_q.head());
        if (lp_md == NULL) {
            lv_fserr = XSRETYPE_NOWORK;
            if (gv_ms_trace_params && gv_ms_trace_verbose)
                trace_where_printf(WHERE,
                                   "EXIT No Work, ret=NOWORK\n");
        } else {
            if (gv_ms_msg_timestamp)
                gettimeofday(&lp_md->iv_ts_msg_srv_listen, NULL);
            lv_fserr = XSRETYPE_IREQ;
            if (pv_big) {
                BMS_SRE *lp_sre = reinterpret_cast<BMS_SRE *>(pp_sre);
                lp_sre->sre_msgId = lp_md->iv_link.iv_id.i;
                lp_sre->sre_flags = BSRE_REMM;
                if (lp_md->out.iv_mon_msg)
                    lp_sre->sre_flags |= BSRE_MON;
                if (lp_md->out.iv_opts & SB_Trans::MS_OPTS_PROCDEAD)
                    lp_sre->sre_flags |= BSRE_PROCDEAD;
                lp_sre->sre_pri = lp_md->out.iv_pri;
                lp_sre->sre_reqCtrlSize = lp_md->out.iv_recv_ctrl_size;
                lp_sre->sre_reqDataSize = lp_md->out.iv_recv_data_size;
                lp_sre->sre_replyCtrlMax = lp_md->out.iv_recv_ctrl_max;
                lp_sre->sre_replyDataMax = lp_md->out.iv_recv_data_max;
                if (gv_ms_trace_params && gv_ms_trace_verbose) {
                    SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                    char *lp_from;
                    if (lp_stream == NULL)
                        lp_from = NULL;
                    else
                        lp_from = lp_stream->get_name();
                    trace_where_printf(WHERE,
                                       "EXIT from=%s, reqid=%d, ret=%d, sre.msgid=%d, .flags=0x%x, .pri=%d, .ctrl=%d, .data=%d\n",
                                       lp_from,
                                       lp_md->out.iv_recv_req_id,
                                       lv_fserr,
                                       lp_sre->sre_msgId,
                                       lp_sre->sre_flags,
                                       lp_sre->sre_pri,
                                       lp_sre->sre_reqCtrlSize,
                                       lp_sre->sre_reqDataSize);
                }
            } else {
                MS_SRE *lp_sre = reinterpret_cast<MS_SRE *>(pp_sre);
                lp_sre->sre_msgId = lp_md->iv_link.iv_id.i;
                lp_sre->sre_flags = XSRE_REMM;
                if (lp_md->out.iv_mon_msg)
                    lp_sre->sre_flags |= XSRE_MON;
                if (lp_md->out.iv_opts & SB_Trans::MS_OPTS_PROCDEAD)
                    lp_sre->sre_flags |= XSRE_PROCDEAD;
                lp_sre->sre_pri = static_cast<short>(lp_md->out.iv_pri);
                lp_sre->sre_reqCtrlSize =
                  static_cast<ushort>(lp_md->out.iv_recv_ctrl_size);
                lp_sre->sre_reqDataSize =
                  static_cast<ushort>(lp_md->out.iv_recv_data_size);
                lp_sre->sre_replyCtrlMax =
                  static_cast<ushort>(lp_md->out.iv_recv_ctrl_max);
                lp_sre->sre_replyDataMax =
                  static_cast<ushort>(lp_md->out.iv_recv_data_max);
                if (gv_ms_trace_params && gv_ms_trace_verbose) {
                    SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                    char *lp_from;
                    if (lp_stream == NULL)
                        lp_from = NULL;
                    else
                        lp_from = lp_stream->get_name();
                    trace_where_printf(WHERE,
                                       "EXIT from=%s, reqid=%d, ret=%d, sre.msgid=%d, .flags=0x%x, .pri=%d, .ctrl=%d, .data=%d\n",
                                       lp_from,
                                       lp_md->out.iv_recv_req_id,
                                       lv_fserr,
                                       lp_sre->sre_msgId,
                                       lp_sre->sre_flags,
                                       lp_sre->sre_pri,
                                       lp_sre->sre_reqCtrlSize,
                                       lp_sre->sre_reqDataSize);
                }
            }
        }
        gv_ms_lim_q.unlock();
        return lv_fserr;
    } else if (pv_listenopts == BLISTEN_TEST_IREQM) {
        if (gv_ms_trace_params && gv_ms_trace_verbose)
            trace_where_printf(WHERE,
                               "ENTER sre=%p, listenopts=0x%x, listenertag=%ld\n",
                               pfp(pp_sre), pv_listenopts, pv_listenertag);
        gv_ms_recv_q.lock();
        lp_md = reinterpret_cast<MS_Md_Type *>(gv_ms_recv_q.head());
        if (lp_md == NULL) {
            lv_fserr = XSRETYPE_NOWORK;
            if (gv_ms_trace_params && gv_ms_trace_verbose)
                trace_where_printf(WHERE,
                                   "EXIT No Work, ret=NOWORK\n");
        } else {
            if (gv_ms_msg_timestamp)
                gettimeofday(&lp_md->iv_ts_msg_srv_listen, NULL);
            lv_fserr = XSRETYPE_IREQ;
            if (pv_big) {
                BMS_SRE *lp_sre = reinterpret_cast<BMS_SRE *>(pp_sre);
                lp_sre->sre_msgId = lp_md->iv_link.iv_id.i;
                lp_sre->sre_flags = BSRE_REMM;
                if (lp_md->out.iv_mon_msg)
                    lp_sre->sre_flags |= BSRE_MON;
                if (lp_md->out.iv_opts & SB_Trans::MS_OPTS_PROCDEAD)
                    lp_sre->sre_flags |= BSRE_PROCDEAD;
                lp_sre->sre_pri = lp_md->out.iv_pri;
                lp_sre->sre_reqCtrlSize = lp_md->out.iv_recv_ctrl_size;
                lp_sre->sre_reqDataSize = lp_md->out.iv_recv_data_size;
                lp_sre->sre_replyCtrlMax = lp_md->out.iv_recv_ctrl_max;
                lp_sre->sre_replyDataMax = lp_md->out.iv_recv_data_max;
                if (gv_ms_trace_params && gv_ms_trace_verbose) {
                    SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                    char *lp_from;
                    if (lp_stream == NULL)
                        lp_from = NULL;
                    else
                        lp_from = lp_stream->get_name();
                    trace_where_printf(WHERE,
                                       "EXIT from=%s, reqid=%d, ret=%d, sre.msgid=%d, .flags=0x%x, .pri=%d, .ctrl=%d, .data=%d\n",
                                       lp_from,
                                       lp_md->out.iv_recv_req_id,
                                       lv_fserr,
                                       lp_sre->sre_msgId,
                                       lp_sre->sre_flags,
                                       lp_sre->sre_pri,
                                       lp_sre->sre_reqCtrlSize,
                                       lp_sre->sre_reqDataSize);
                }
            } else {
                MS_SRE *lp_sre = reinterpret_cast<MS_SRE *>(pp_sre);
                lp_sre->sre_msgId = lp_md->iv_link.iv_id.i;
                lp_sre->sre_flags = XSRE_REMM;
                if (lp_md->out.iv_mon_msg)
                    lp_sre->sre_flags |= XSRE_MON;
                if (lp_md->out.iv_opts & SB_Trans::MS_OPTS_PROCDEAD)
                    lp_sre->sre_flags |= XSRE_PROCDEAD;
                lp_sre->sre_pri = static_cast<short>(lp_md->out.iv_pri);
                lp_sre->sre_reqCtrlSize =
                  static_cast<ushort>(lp_md->out.iv_recv_ctrl_size);
                lp_sre->sre_reqDataSize =
                  static_cast<ushort>(lp_md->out.iv_recv_data_size);
                lp_sre->sre_replyCtrlMax =
                  static_cast<ushort>(lp_md->out.iv_recv_ctrl_max);
                lp_sre->sre_replyDataMax =
                  static_cast<ushort>(lp_md->out.iv_recv_data_max);
                if (gv_ms_trace_params && gv_ms_trace_verbose) {
                    SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                    char *lp_from;
                    if (lp_stream == NULL)
                        lp_from = NULL;
                    else
                        lp_from = lp_stream->get_name();
                    trace_where_printf(WHERE,
                                       "EXIT from=%s, reqid=%d, ret=%d, sre.msgid=%d, .flags=0x%x, .pri=%d, .ctrl=%d, .data=%d\n",
                                       lp_from,
                                       lp_md->out.iv_recv_req_id,
                                       lv_fserr,
                                       lp_sre->sre_msgId,
                                       lp_sre->sre_flags,
                                       lp_sre->sre_pri,
                                       lp_sre->sre_reqCtrlSize,
                                       lp_sre->sre_reqDataSize);
                }
            }
        }
        gv_ms_recv_q.unlock();
        return lv_fserr;
    }

    do {
        lv_fserr = -1;
        lv_repeat = false;
        if (pv_listenopts & BLISTEN_ALLOW_ABANDONM) {
            // abandon check requested, check abandon
            lp_md = static_cast<MS_Md_Type *>(gv_ms_abandon_comp_q.remove());
            if (lp_md != NULL) {
                lv_abandoned = true;
                break; // finish abandon
            }
        }
        if (pv_listenopts & BLISTEN_ALLOW_LDONEM) {
            // ldone check requested, check ldone
            lp_md = static_cast<MS_Md_Type *>(ms_ldone_get_comp_q()->remove());
            if (lp_md != NULL)
                break; // finish ldone
        }
        if (pv_listenopts & BLISTEN_ALLOW_TPOPM) {
            // tpop check requested, check tpop
            lp_tle = sb_timer_comp_q_remove();
            if (lp_tle != NULL)
                break; // finish tpop
        }
        if (!(pv_listenopts & (BLISTEN_ALLOW_ILIMREQM | BLISTEN_ALLOW_IREQM))) {
            // NO ireq check requested, so we're done
            lv_fserr = XSRETYPE_NOWORK;
            break;
        }
        if (pv_listenopts & BLISTEN_ALLOW_ILIMREQM) {
            // check ilimreqs
            lp_md = static_cast<MS_Md_Type *>(gv_ms_lim_q.remove());
        }
        if ((lp_md == NULL) && (pv_listenopts & BLISTEN_ALLOW_IREQM)) {
            // check ireqs
            lp_md = static_cast<MS_Md_Type *>(gv_ms_recv_q.remove());
        }
        if (lp_md == NULL) {
            lv_fserr = XSRETYPE_NOWORK;
            break;
        }
        if (gv_ms_msg_timestamp)
            gettimeofday(&lp_md->iv_ts_msg_srv_listen, NULL);
        if (lp_md->out.iv_mon_msg) {
            if (!lv_traced && gv_ms_trace_params && !gv_ms_trace_verbose) {
                lv_traced = true;
                trace_where_printf(WHERE,
                                   "ENTER sre=%p, listenopts=0x%x, listenertag=%ld\n",
                                   pfp(pp_sre), pv_listenopts, pv_listenertag);
            }
            msg_mon_recv_msg(lp_md);
            if (!gv_ms_enable_messages) {
                lv_repeat = true;
                if (gv_ms_trace_mon)
                    trace_where_printf(WHERE,
                                       "disabled mon-messages, deleting mon message, sre.msgid=%d\n",
                                       lp_md->iv_link.iv_id.i);
                MS_BUF_MGR_FREE(lp_md->out.ip_recv_data);
                lp_md->out.ip_recv_data = NULL;
                SB_Trans::Msg_Mgr::put_md(lp_md->iv_link.iv_id.i, "mon-msg disabled");
            }
        }
    } while (lv_repeat);
    // set LREQ according to whether there are more messages [timers]
    gv_ms_recv_q.lock(); // good for timer too
    if (gv_ms_recv_q.empty() && sb_timer_comp_q_empty())
        gv_ms_event_mgr.get_mgr(NULL)->clear_event(LREQ);
    else
        gv_ms_event_mgr.get_mgr(NULL)->set_event(LREQ, NULL);
    gv_ms_recv_q.unlock();
    if (pv_listenopts & BLISTEN_ALLOW_ILIMREQM) {
        // repost INTR if necessary
        gv_ms_lim_q.lock();
        if (gv_ms_lim_q.empty())
            gv_ms_event_mgr.get_mgr(NULL)->clear_event(INTR);
        else
            gv_ms_event_mgr.get_mgr(NULL)->set_event(INTR, NULL);
        gv_ms_lim_q.unlock();
    }
    // set LCAN if there are more cancel messages (don't clear if there aren't)
    if (!gv_ms_abandon_comp_q.empty())
        gv_ms_event_mgr.get_mgr(NULL)->set_event(LCAN, NULL);
    if (lv_fserr == XSRETYPE_NOWORK) {
        if (gv_ms_trace_verbose)
            return ms_err_rtn_msg(WHERE,
                                  "EXIT No work, ret=NOWORK",
                                  lv_fserr);
        else
            return lv_fserr;
    }
    if (!lv_traced && gv_ms_trace_params && !gv_ms_trace_verbose)
        trace_where_printf(WHERE,
                           "ENTER sre=%p, listenopts=0x%x, listenertag=%ld\n",
                           pfp(pp_sre), pv_listenopts, pv_listenertag);
    if (lp_tle == NULL)
        lp_md->ip_mgr = gv_ms_event_mgr.get_mgr(NULL);
    if (lp_tle != NULL) {
        BMS_SRE_TPOP *lp_sre_tpop = reinterpret_cast<BMS_SRE_TPOP *>(pp_sre);
        sb_timer_set_sre_tpop(lp_sre_tpop, lp_tle);
        lv_fserr = XSRETYPE_TPOP;
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_TPOP,
                           lp_sre_tpop->sre_tleId,
                           lv_fserr);
        if (gv_ms_trace_params)
            trace_where_printf(WHERE,
                               "EXIT from=self(tpop), ret=%d, sre.tleid=%d, .toval=%d, .p1=%d, .p2=%ld\n",
                               lv_fserr,
                               lp_sre_tpop->sre_tleId,
                               lp_sre_tpop->sre_tleTOVal,
                               lp_sre_tpop->sre_tleParm1,
                               lp_sre_tpop->sre_tleParm2);
    } else if (lv_abandoned) {
        BMS_SRE_ABANDON *lp_sre_abandon = reinterpret_cast<BMS_SRE_ABANDON *>(pp_sre);
        lp_sre_abandon->sre_msgId = lp_md->iv_link.iv_id.i;
        lp_sre_abandon->sre_servTag = lp_md->iv_tag;
        lv_fserr = BSRETYPE_ABANDON;
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_ABANDON,
                           lp_md->iv_link.iv_id.i,
                           lv_fserr);
        if (gv_ms_trace_params)
            trace_where_printf(WHERE,
                               "EXIT from=self(abandon), ret=%d, sre.msgid=%d, sre.servTag=" PFTAG "\n",
                               lv_fserr,
                               lp_sre_abandon->sre_msgId,
                               lp_sre_abandon->sre_servTag);
    } else if (lp_md->out.iv_ldone) {
        BMS_SRE_LDONE *lp_sre_ldone = reinterpret_cast<BMS_SRE_LDONE *>(pp_sre);
        lp_sre_ldone->sre_msgId = lp_md->iv_link.iv_id.i;
        lp_sre_ldone->sre_linkTag = lp_md->iv_tag;
        lv_fserr = XSRETYPE_LDONE;
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_LDONE,
                           lp_md->iv_link.iv_id.i,
                           lv_fserr);
        if (gv_ms_trace_params)
            trace_where_printf(WHERE,
                               "EXIT from=self(ldone), ret=%d, sre.msgid=%d, sre.linkTag=" PFTAG "\n",
                               lv_fserr,
                               lp_sre_ldone->sre_msgId,
                               lp_sre_ldone->sre_linkTag);
    } else {
        // unfortunately, the code is replicated due to different
        // structs being used
        lp_md->iv_tag = pv_listenertag;
        if (pv_big) {
            BMS_SRE *lp_sre = reinterpret_cast<BMS_SRE *>(pp_sre);
            lp_sre->sre_msgId = lp_md->iv_link.iv_id.i;
            lp_sre->sre_flags = BSRE_REMM;
            if (lp_md->out.iv_mon_msg)
                lp_sre->sre_flags |= BSRE_MON;
            if (lp_md->out.iv_opts & SB_Trans::MS_OPTS_PROCDEAD)
                lp_sre->sre_flags |= BSRE_PROCDEAD;
            lp_sre->sre_pri = lp_md->out.iv_pri;
            lp_sre->sre_reqCtrlSize = lp_md->out.iv_recv_ctrl_size;
            lp_sre->sre_reqDataSize = lp_md->out.iv_recv_data_size;
            lp_sre->sre_replyCtrlMax = lp_md->out.iv_recv_ctrl_max;
            lp_sre->sre_replyDataMax = lp_md->out.iv_recv_data_max;
            lv_fserr = XSRETYPE_IREQ;
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_IREQ,
                               lp_md->iv_link.iv_id.i,
                               lv_fserr);
            if (lp_md->out.iv_recv_ctrl_size > 0)
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_IREQ_CTRL,
                                   lp_md->out.iv_recv_ctrl_size,
                                   reinterpret_cast<long>(lp_md->out.ip_recv_ctrl));
            if (lp_md->out.iv_recv_data_size > 0)
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_IREQ_DATA,
                                   lp_md->out.iv_recv_data_size,
                                   reinterpret_cast<long>(lp_md->out.ip_recv_data));
            if (gv_ms_trace_params) {
                SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                char *lp_from;
                if (lp_stream == NULL)
                    lp_from = NULL;
                else
                    lp_from = lp_stream->get_name();
                trace_where_printf(WHERE,
                                   "EXIT from=%s, reqid=%d, ret=%d, sre.msgid=%d, .flags=0x%x, .pri=%d, .ctrl=%d, .data=%d\n",
                                   lp_from,
                                   lp_md->out.iv_recv_req_id,
                                   lv_fserr,
                                   lp_sre->sre_msgId,
                                   lp_sre->sre_flags,
                                   lp_sre->sre_pri,
                                   lp_sre->sre_reqCtrlSize,
                                   lp_sre->sre_reqDataSize);
            }
        } else {
            MS_SRE *lp_sre = reinterpret_cast<MS_SRE *>(pp_sre);
            lp_sre->sre_msgId = lp_md->iv_link.iv_id.i;
            lp_sre->sre_flags = XSRE_REMM;
            if (lp_md->out.iv_mon_msg)
                lp_sre->sre_flags |= XSRE_MON;
            if (lp_md->out.iv_opts & SB_Trans::MS_OPTS_PROCDEAD)
                lp_sre->sre_flags |= XSRE_PROCDEAD;
            lp_sre->sre_pri = static_cast<short>(lp_md->out.iv_pri);
            lp_sre->sre_reqCtrlSize =
              static_cast<ushort>(lp_md->out.iv_recv_ctrl_size);
            lp_sre->sre_reqDataSize =
              static_cast<ushort>(lp_md->out.iv_recv_data_size);
            lp_sre->sre_replyCtrlMax =
              static_cast<ushort>(lp_md->out.iv_recv_ctrl_max);
            lp_sre->sre_replyDataMax =
              static_cast<ushort>(lp_md->out.iv_recv_data_max);
            lv_fserr = XSRETYPE_IREQ;
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_IREQ,
                               lp_md->iv_link.iv_id.i,
                               lv_fserr);
            if (lp_md->out.iv_recv_ctrl_size > 0)
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_IREQ_CTRL,
                                   lp_md->out.iv_recv_ctrl_size,
                                   reinterpret_cast<long>(lp_md->out.ip_recv_ctrl));
            if (lp_md->out.iv_recv_data_size > 0)
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MSG_LISTEN_IREQ_DATA,
                                   lp_md->out.iv_recv_data_size,
                                   reinterpret_cast<long>(lp_md->out.ip_recv_data));
            if (gv_ms_trace_params) {
                SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                char *lp_from;
                if (lp_stream == NULL)
                    lp_from = NULL;
                else
                    lp_from = lp_stream->get_name();
                trace_where_printf(WHERE,
                                   "EXIT from=%s, reqid=%d, ret=%d, sre.msgid=%d, .flags=0x%x, .pri=%d, .ctrl=%d, .data=%d\n",
                                   lp_from,
                                   lp_md->out.iv_recv_req_id,
                                   lv_fserr,
                                   lp_sre->sre_msgId,
                                   lp_sre->sre_flags,
                                   lp_sre->sre_pri,
                                   lp_sre->sre_reqCtrlSize,
                                   lp_sre->sre_reqDataSize);
            }
        }
    }
    return lv_fserr;
}

SB_Export short BMSG_LISTEN_(short *pp_sre,
                             short  pv_listenopts,
                             long   pv_listenertag) {
    return BMSG_LISTEN_common(pp_sre, pv_listenopts, pv_listenertag, true);
}

//
// Purpose: emulate READCTRL_
//
SB_Export short BMSG_READCTRL_(int    pv_msgid,
                               short *pp_reqctrl,
                               int    pv_bytecount) {
    const char *WHERE = "BMSG_READCTRL_";
    MS_Md_Type *lp_md;
    int         lv_rc;
    SB_API_CTR (lv_zctr, BMSG_READCTRL_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER msgid=%d, reqC=%p, bytecount=%d\n",
                           pv_msgid, pfp(pp_reqctrl), pv_bytecount);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, WHERE);
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    if (lp_md->iv_abandoned)
        return ms_err_rtn_msg(WHERE, "EXIT [msg abandoned]", 1); // spec says 1
        SB_Trans::Trans_Stream::recv_copy(WHERE,
                                          &lp_md->out.ip_recv_ctrl,
                                          lp_md->out.iv_recv_ctrl_size,
                                          reinterpret_cast<char *>(pp_reqctrl),
                                          pv_bytecount,
                                          &lv_rc,
                                          false);
    if (gv_ms_trace_data) {
        trace_where_printf(WHERE,
                           "reqC=%p, bytecount=%d\n",
                           pfp(pp_reqctrl), lv_rc);
        trace_print_data(pp_reqctrl, lv_rc, gv_ms_trace_data_max);
    }
    short lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: emulate MSG_READDATA_
//
SB_Export short BMSG_READDATA_(int     pv_msgid,
                               char   *pp_reqdata,
                               int     pv_bytecount) {
    const char *WHERE = "BMSG_READDATA_";
    MS_Md_Type *lp_md;
    int         lv_rc;
    SB_API_CTR (lv_zctr, BMSG_READDATA_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER msgid=%d, reqD=%p, bytecount=%d\n",
                           pv_msgid, pp_reqdata, pv_bytecount);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, WHERE);
    if (lp_md == NULL)
        return ms_err_rtn_msg(WHERE, "EXIT [msg == NULL]", XZFIL_ERR_NOTFOUND);
    if (lp_md->iv_abandoned)
        return ms_err_rtn_msg(WHERE, "EXIT [msg abandoned]", 1); // spec says 1
        SB_Trans::Trans_Stream::recv_copy(WHERE,
                                          &lp_md->out.ip_recv_data,
                                          lp_md->out.iv_recv_data_size,
                                          pp_reqdata,
                                          pv_bytecount,
                                          &lv_rc,
                                          false);
    if (gv_ms_trace_data) {
        trace_where_printf(WHERE,
                           "reqD=%p, bytecount=%d\n",
                           pp_reqdata, lv_rc);
        trace_print_data(pp_reqdata, lv_rc, gv_ms_trace_data_max);
    }
    short lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: emulate MSG_RELEASEALLHELD_
//
SB_Export void BMSG_RELEASEALLHELD_() {
    const char *WHERE = "BMSG_RELEASEALLHELD_";
    SB_API_CTR (lv_zctr, BMSG_RELEASEALLHELD_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    // NSK would do a halt
    SB_util_assert_it(gv_ms_calls_ok); // sw fault

    // move hold-q to recv-q
    MS_Md_Type *lp_md = static_cast<MS_Md_Type *>(gv_ms_hold_q.remove());
    while (lp_md != NULL) {
        gv_ms_recv_q.add(&lp_md->iv_link);
        lp_md = static_cast<MS_Md_Type *>(gv_ms_hold_q.remove());
    }

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// Purpose: emulate MSG_REPLY_
//
SB_Export void BMSG_REPLY_(int              pv_msgid,
                           short           *pp_replyctrl,
                           int              pv_replyctrlsize,
                           char            *pp_replydata,
                           int              pv_replydatasize,
                           short            pv_errorclass,
                           SB_Phandle_Type *pp_newphandle) {
    const char *WHERE = "BMSG_REPLY_";
    long long   lv_delta;
    short       lv_fserr;
    SB_API_CTR (lv_zctr, BMSG_REPLY_);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_MSG_REPLY, pv_msgid);
    MS_Md_Type *lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, WHERE);
    if (lp_md == NULL) {
        if (gv_ms_trace_params) {
            trace_where_printf(WHERE, "ENTER msgid=%d, repC=%d, repD=%d\n",
                               pv_msgid, pv_replyctrlsize, pv_replydatasize);
            trace_where_printf(WHERE, "EXIT unknown msgid=%d\n", pv_msgid);
        }
        // NSK does a halt
        SB_util_assert_pne(lp_md, NULL); // sw fault
    }
    SB_Trans::Stream_Base *lp_stream = static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
    if (gv_ms_trace_params) {
        char *lp_to;
        if (lp_stream == NULL)
            lp_to = NULL;
        else
            lp_to = lp_stream->get_name();
        trace_where_printf(WHERE,
                           "ENTER to=%s (reqid=%d), msgid=%d, repC=%p, repCsize=%d\n",
                           lp_to,
                           lp_md->out.iv_recv_req_id,
                           pv_msgid,
                           pfp(pp_replyctrl),
                           pv_replyctrlsize);
        gettimeofday(&lp_md->out.iv_reply_tod, NULL);
        lv_delta = (lp_md->out.iv_reply_tod.tv_sec * SB_US_PER_SEC +
                    lp_md->out.iv_reply_tod.tv_usec) -
                   (lp_md->out.iv_recv_q_off_tod.tv_sec * SB_US_PER_SEC +
                    lp_md->out.iv_recv_q_off_tod.tv_usec);
        trace_where_printf(WHERE,
                           "repD=%p, repDsize=%d, ecls=%d, nphdl=%p, listen-to-reply=%lld us\n",
                           pfp(pp_replydata),
                           pv_replydatasize,
                           pv_errorclass,
                           pfp(pp_newphandle),
                           lv_delta);
    }
    if (gv_ms_trace_data) {
        trace_where_printf(WHERE,
                           "repC=%p, repCsize=%d\n",
                           pfp(pp_replyctrl), pv_replyctrlsize);
        trace_print_data(pp_replyctrl, pv_replyctrlsize, gv_ms_trace_data_max);
        trace_where_printf(WHERE,
                           "repD=%p, repDsize=%d\n",
                           pp_replydata, pv_replydatasize);
        trace_print_data(pp_replydata, pv_replydatasize, gv_ms_trace_data_max);
    }
    if (lp_md->out.iv_mon_msg) {
        if (lp_md->out.iv_requeue) {
            lp_md->out.iv_requeue = false;
            gv_ms_recv_q.add_at_front(&lp_md->iv_link);
            gv_ms_event_mgr.set_event_all(LREQ);
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "EXIT mon message requeue\n");
        } else {
            // could be timer (no stream)
            ms_free_recv_bufs(lp_md);
            if (lp_stream == NULL)
                SB_Trans::Msg_Mgr::put_md(lp_md->iv_link.iv_id.i, "free mon-reply-md");
            else
                lp_stream->free_reply_md(lp_md);
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "EXIT mon message\n");
        }
        return;
    }
    if (!lp_md->out.iv_reply) {
        ms_free_recv_bufs(lp_md);
        if (lp_stream == NULL)
            SB_Trans::Msg_Mgr::put_md(lp_md->iv_link.iv_id.i, "free no-reply-md");
        else
            lp_stream->free_reply_md(lp_md);
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT no reply\n");
        return;
    }
    // if client limits max, then we need to limit our response
    int lv_csize = sbmin(pv_replyctrlsize, lp_md->out.iv_recv_ctrl_max);
    int lv_dsize = sbmin(pv_replydatasize, lp_md->out.iv_recv_data_max);
    lv_fserr = lp_stream->exec_reply(lp_md,
                                     0,                                  // src
                                     lp_md->out.iv_recv_mpi_source_rank, // dest
                                     lp_md->out.iv_recv_req_id,
                                     pp_replyctrl,
                                     lv_csize,
                                     pp_replydata,
                                     lv_dsize,
                                     XZFIL_ERR_OK);
    // if pathdown, don't worry about it
    ms_err_check_mpi_pathdown_ok(WHERE, "sending reply", lv_fserr);
    if (lv_fserr == XZFIL_ERR_OK)
        lp_stream->wait_rep_done(lp_md); // don't wait if path is down
    // free recv buffers after reply in case user uses them in reply
    ms_free_recv_bufs(lp_md);
    lp_stream->free_reply_md(lp_md);
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// Purpose: emulate MSG_SETTAG_
//
SB_Export short BMSG_SETTAG_(int  pv_msgid,
                             long pv_tag) {
    const char *WHERE = "BMSG_SETTAG_";
    SB_API_CTR (lv_zctr, BMSG_SETTAG_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER msgid=%d, tag=%ld\n",
                           pv_msgid,
                           pv_tag);
    MS_Md_Type *lp_md = SB_Trans::Msg_Mgr::map_to_md(pv_msgid, WHERE);
    if (lp_md == NULL) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT unknown msgid=%d\n", pv_msgid);
        SB_util_assert_pne(lp_md, NULL); // sw fault
    }
    lp_md->iv_tag = pv_tag;
    short lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

SB_Export short XCONTROLMESSAGESYSTEM(short pv_actioncode, short pv_value) {
    const char *WHERE = "XCONTROLMESSAGESYSTEM";
    short       lv_fserr;
    SB_API_CTR (lv_zctr, XCONTROLMESSAGESYSTEM);

    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_CONTROLMESSAGESYSTEM,
                       pv_actioncode, pv_value);
    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER actioncode=%d, value=%d\n",
                           pv_actioncode, pv_value);

    switch (pv_actioncode) {
    case XCTLMSGSYS_SETRECVLIMIT:
        if ((pv_value > 0) && (pv_value <= XMAX_SETTABLE_RECVLIMIT_TM)) {
            lv_fserr = XZFIL_ERR_OK;
            SB_Trans::Msg_Mgr::set_md_max_recv(pv_value);
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "setting SETRECVLIMIT value=%d\n",
                                   pv_value);
        } else {
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "EXIT bad value\n");
            lv_fserr = XZFIL_ERR_BADCOUNT;
        }
        break;

    case XCTLMSGSYS_SETSENDLIMIT:
        if ((pv_value > 0) && (pv_value <= XMAX_SETTABLE_SENDLIMIT_TM)) {
            lv_fserr = XZFIL_ERR_OK;
            SB_Trans::Msg_Mgr::set_md_max_send(pv_value);
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "setting SETSENDLIMIT value=%d\n",
                                   pv_value);
        } else {
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "EXIT bad value\n");
            lv_fserr = XZFIL_ERR_BADCOUNT;
        }
        break;

    default:
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT bad actioncode\n");
        lv_fserr = XZFIL_ERR_INVALOP;
        break;
    }

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

SB_Export short XMESSAGESYSTEMINFO(short pv_itemcode, short *pp_value) {
    const char *WHERE = "XMESSAGESYSTEMINFO";
    short       lv_fserr;
    short       lv_value;
    SB_API_CTR (lv_zctr, XMESSAGESYSTEMINFO);

    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MESSAGESYSTEMINFO, pv_itemcode, 0);
    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER itemcode=%d, value=%p\n",
                           pv_itemcode, pfp(pp_value));

    if (pp_value == NULL) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT bad value\n");
        lv_fserr = XZFIL_ERR_BADCOUNT;
    } else {
        lv_fserr = XZFIL_ERR_OK;
        switch (pv_itemcode) {
        case XMSGSYSINFO_RECVLIMIT:
            lv_value = static_cast<short>(SB_Trans::Msg_Mgr::get_md_max_recv());
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "returning RECVLIMIT value=%d\n",
                                   lv_value);
            break;

        case XMSGSYSINFO_SENDLIMIT:
            lv_value = static_cast<short>(SB_Trans::Msg_Mgr::get_md_max_send());
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "returning SENDLIMIT value=%d\n",
                                   lv_value);
            break;

        case XMSGSYSINFO_RECVUSE:
            lv_value = static_cast<short>(SB_Trans::Msg_Mgr::get_md_count_recv());
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "returning RECVUSE value=%d\n",
                                   lv_value);
            break;

        case XMSGSYSINFO_SENDUSE:
            lv_value = static_cast<short>(SB_Trans::Msg_Mgr::get_md_count_send());
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "returning SENDUSE value=%d\n",
                                   lv_value);
            break;

        case XMSGSYSINFO_RECVSIZE:
            lv_value = static_cast<short>(gv_ms_recv_q.size());
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "returning RECVSIZE value=%d\n",
                                   lv_value);
            break;

        case XMSGSYSINFO_RECVLIMSIZE:
            lv_value = static_cast<short>(gv_ms_lim_q.size());
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "returning RECVLIMSIZE value=%d\n",
                                   lv_value);
            break;

        default:
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "EXIT bad itemcode\n");
            lv_fserr = XZFIL_ERR_INVALOP;
            break;
        }
        if (lv_fserr == XZFIL_ERR_OK) {
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_MS_MESSAGESYSTEMINFO,
                               pv_itemcode, lv_value);
            *pp_value = lv_value;
        }
    }

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

SB_Export short XMSG_ABANDON_(int pv_msgid) {
    return BMSG_ABANDON_(pv_msgid);
}

SB_Export short XMSG_AWAIT_(int pv_msgid, int pv_tov) {
    return BMSG_AWAIT_(pv_msgid, pv_tov);
}

SB_Export short XMSG_BREAK_(int              pv_msgid,
                            short           *pp_results,
                            SB_Phandle_Type *pp_phandle) {
    return BMSG_BREAK_(pv_msgid,
                       pp_results,
                       pp_phandle);
}

SB_Export void XMSG_BREAK2_(int              pv_msgid,
                            short           *pp_results,
                            SB_Phandle_Type *pp_phandle) {
    BMSG_BREAK2_(pv_msgid,
                 pp_results,
                 pp_phandle);
}

SB_Export short XMSG_GETREQINFO_(int     pv_item_code,
                                 int     pv_msgid,
                                 int    *pp_item) {
    return BMSG_GETREQINFO_(pv_item_code,
                            pv_msgid,
                            pp_item);
}

SB_Export void XMSG_HOLD_(int pv_msgid) {
    BMSG_HOLD_(pv_msgid);
}

SB_Export short XMSG_ISCANCELED_(int pv_msgid) {
    return BMSG_ISCANCELED_(pv_msgid);
}

SB_Export short XMSG_ISDONE_(int pv_msgid) {
    return BMSG_ISDONE_(pv_msgid);
}

SB_Export short XMSG_LINK_(SB_Phandle_Type *pp_phandle,
                           int             *pp_msgid,
                           short           *pp_reqctrl,
                           ushort           pv_reqctrlsize,
                           short           *pp_replyctrl,
                           ushort           pv_replyctrlmax,
                           char            *pp_reqdata,
                           ushort           pv_reqdatasize,
                           char            *pp_replydata,
                           ushort           pv_replydatamax,
                           long             pv_linkertag,
                           short            pv_pri,
                           short            pv_xmitclass,
                           short            pv_linkopts) {
    return BMSG_LINK_common(pp_phandle,
                            pp_msgid,
                            pp_reqctrl,
                            pv_reqctrlsize,
                            pp_replyctrl,
                            pv_replyctrlmax,
                            pp_reqdata,
                            pv_reqdatasize,
                            pp_replydata,
                            pv_replydatamax,
                            pv_linkertag,
                            pv_pri,
                            pv_xmitclass,
                            pv_linkopts,
                            false);
}

SB_Export short XMSG_LINK2_(SB_Phandle_Type *pp_phandle,
                            int             *pp_msgid,
                            short           *pp_reqctrl,
                            ushort           pv_reqctrlsize,
                            short           *pp_replyctrl,
                            ushort           pv_replyctrlmax,
                            char            *pp_reqdata,
                            ushort           pv_reqdatasize,
                            char            *pp_replydata,
                            ushort           pv_replydatamax,
                            long             pv_linkertag,
                            short            pv_pri,
                            short            pv_xmitclass,
                            short            pv_linkopts) {
    return BMSG_LINK_common(pp_phandle,
                            pp_msgid,
                            pp_reqctrl,
                            pv_reqctrlsize,
                            pp_replyctrl,
                            pv_replyctrlmax,
                            pp_reqdata,
                            pv_reqdatasize,
                            pp_replydata,
                            pv_replydatamax,
                            pv_linkertag,
                            pv_pri,
                            pv_xmitclass,
                            pv_linkopts,
                            true); // nsk check
}

SB_Export short XMSG_LISTEN_(short *pp_sre,
                             short  pv_listenopts,
                             long   pv_listenertag) {
    return BMSG_LISTEN_common(pp_sre,
                              pv_listenopts,
                              pv_listenertag,
                              false);
}

SB_Export short XMSG_READCTRL_(int     pv_msgid,
                               short  *pp_reqctrl,
                               ushort  pv_bytecount) {
    return BMSG_READCTRL_(pv_msgid,
                          pp_reqctrl,
                          pv_bytecount);
}

SB_Export short XMSG_READDATA_(int     pv_msgid,
                               char   *pp_reqdata,
                               ushort  pv_bytecount) {
    return BMSG_READDATA_(pv_msgid,
                          pp_reqdata,
                          pv_bytecount);
}

SB_Export void XMSG_REPLY_(int              pv_msgid,
                           short           *pp_replyctrl,
                           ushort           pv_replyctrlsize,
                           char            *pp_replydata,
                           ushort           pv_replydatasize,
                           short            pv_errorclass,
                           SB_Phandle_Type *pp_newphandle) {
    return BMSG_REPLY_(pv_msgid,
                       pp_replyctrl,
                       pv_replyctrlsize,
                       pp_replydata,
                       pv_replydatasize,
                       pv_errorclass,
                       pp_newphandle);
}

SB_Export void  XMSG_RELEASEALLHELD_() {
    BMSG_RELEASEALLHELD_();
}

SB_Export short XMSG_SETTAG_(int pv_msgid, long pv_tag) {
    return BMSG_SETTAG_(pv_msgid, pv_tag);
}

//
// Purpose: send-to-self
//          format md and queue to $receive
//          no reply allowed
//
void msg_send_self(SB_Phandle_Type *pp_phandle,
                   short           *pp_reqctrl,
                   int              pv_reqctrlsize,
                   char            *pp_reqdata,
                   int              pv_reqdatasize) {
    const char            *WHERE = "msg_send_self";
    MS_Md_Type            *lp_md;
    SB_Trans::Stream_Base *lp_stream;

    lp_stream = ms_od_map_phandle_to_stream(pp_phandle);
    SB_Trans::Msg_Mgr::get_md(&lp_md, // msg_send_self
                              lp_stream,
                              NULL,
                              false,  // recv
                              NULL,   // fserr
                              WHERE,
                              MD_STATE_MSG_SEND_SELF);
    SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
    lp_md->iv_tag = -1;
    lp_md->iv_self = true;
    lp_md->out.iv_mon_msg = false;
    lp_md->out.iv_nid = gv_ms_su_nid;
    lp_md->out.iv_opts = 0;
    lp_md->out.iv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_md->out.iv_verif = gv_ms_su_verif;
#endif
    lp_md->out.iv_recv_req_id = 0;
    lp_md->out.iv_pri = -1;
    lp_md->out.iv_recv_mpi_source_rank = -1;
    lp_md->out.iv_recv_mpi_tag = -1;
    lp_md->out.ip_recv_ctrl = reinterpret_cast<char *>(pp_reqctrl);
    lp_md->out.iv_recv_ctrl_size = pv_reqctrlsize;
    lp_md->out.ip_reply_ctrl = NULL;
    lp_md->out.iv_reply_ctrl_max = 0;
    lp_md->out.ip_recv_data = pp_reqdata;
    lp_md->out.iv_recv_data_size = pv_reqdatasize;
    lp_md->out.ip_reply_data = NULL;
    lp_md->out.iv_reply_data_max = 0;
    gv_ms_recv_q.add(&lp_md->iv_link);
    gv_ms_event_mgr.set_event_all(LREQ);
}

SB_Export short msg_set_phandle(char            *pp_pname,
                                SB_Phandle_Type *pp_phandle) {
    const char      *WHERE = "msg_set_phandle";
    char             la_pname[MS_MON_MAX_PROCESS_NAME+1];
    SB_Phandle_Type *lp_phandle;
    short            lv_fserr;
    SB_API_CTR      (lv_zctr, MSG_SET_PHANDLE);

    if (gv_ms_trace_params) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, pp_phandle);
        trace_where_printf(WHERE,
                           "ENTER pname=%s, phandle=%s\n",
                           pp_pname,
                           la_phandle);
    }
    if (pp_pname == NULL) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT ret=-1, name is NULL\n");
        return -1;
    }

    if (gv_ms_init_phandle) {
        gv_ms_init_phandle = false;
        gv_ms_max_phandles = 0;
        ms_getenv_int(gp_ms_env_max_cap_phandles, &gv_ms_max_phandles);
    }

    SB_util_get_case_insensitive_name(pp_pname, la_pname);

    if (pp_phandle == NULL) {
        gv_ms_phandle_map.remove(la_pname, NULL);
    } else {
        if (gv_ms_trace)
            trace_where_printf(WHERE, "PHANDLES-inuse-count=%d\n",
                               gv_ms_phandle_map.size());
        if (gv_ms_max_phandles)
            SB_util_assert_ilt(gv_ms_phandle_map.size(), gv_ms_max_phandles);
        lp_phandle = new SB_Phandle_Type;
        memcpy(lp_phandle, pp_phandle, sizeof(SB_Phandle_Type));
        gv_ms_phandle_map.putv(la_pname, lp_phandle);
    }
    lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return lv_fserr;
}

int msg_test_assert_disable() {
    int lv_aret = gv_ms_assert_error;
    gv_ms_assert_error = 0;
    return lv_aret;
}

void msg_test_assert_enable(int pv_state) {
    gv_ms_assert_error = pv_state;
}

void msg_trace_args(char *pp_line, char **ppp_argv, int pv_argc, int pv_len) {
    int lv_len = static_cast<int>(strlen(pp_line));
    for (int lv_arg = 0; lv_arg < pv_argc; lv_arg++) {
        lv_len += 3 + static_cast<int>(strlen(ppp_argv[lv_arg]));
        if (lv_len > pv_len)
            break;
        strcat(pp_line, "'");
        strcat(pp_line, ppp_argv[lv_arg]);
        strcat(pp_line, "'");
        if (lv_arg != (pv_argc - 1))
            strcat(pp_line, ",");
    }
}

//
// For future use - called when library is loaded
// Note that other constructors may have been called
// before this one gets called.
//
void __msg_init(void) {
}

//
// For future use - called when library is unloaded
//
void __msg_fini(void) {
}

