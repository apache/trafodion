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
// suspend/resume threads
//

//
// strategy...
//    create m2s - master-to-slave pipe
//    create s2m - slave-to-master pipe
//    where master is thread doing the suspend/resume
//    where slave are thread(s) getting suspended/resumed
//
// thread_suspend_all -
//   for (each-thread-except-self) {
//       tgkill(thread, SIGURG); // tell thread to suspend
//       pipe_read(s2m);         // wait for ack
//   }
//
// thread_resume_suspended -
//   for (each-thread-except-self) {
//       pipe_write(m2s); // tell thread to resume
//       pipe_read(s2m);  // wait for ack
//   }
//
//   At each-thread-except-self:
//     will take thr_int_rs_sig, which will then:
//       // must do in async-safe manner
//       pipe_write(s2m);  // ack-suspend
//       pipe_read(m2s);   // wait for resume
//       pipe_write(s2m);  // ack-resume
//

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <linux/unistd.h>
#include <sys/stat.h>

#include "seabed/fserr.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "mserrmsg.h"
#include "mstrace.h"
#include "msx.h"
#include "util.h"

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))
#define tgkill(pv_pid,pv_tid,pv_sig) syscall(__NR_tgkill,pv_pid,pv_tid,pv_sig)


enum { THR_RS_MAX_THREADS = 50 };

enum { THR_RS_PIPE_READ   = 0 };
enum { THR_RS_PIPE_WRITE  = 1 };

//
// pipe
//
typedef struct Thr_Rs_Pipe {
    int         ia_errno[2];
    int         ia_pipe[2];
    const char *ip_desc;
} Thr_Rs_Pipe;

//
// action node
//
typedef enum {
    THR_RS_RESUME  = 1,
    THR_RS_SUSPEND = 2
} Thr_Rs_Action;

//
// thread node
//
typedef struct Thr_Rs_Thread {
    int iv_tid;
} Thr_Rs_Thread;

//
// state
//
typedef struct Thr_Rs_State {
    int            iv_inx;
    Thr_Rs_Action  iv_state;
    Thr_Rs_Thread  ia_threads[THR_RS_MAX_THREADS];
    Thr_Rs_Pipe    iv_m2s_pipe;
    Thr_Rs_Pipe    iv_s2m_pipe;
} Thr_Rs_State;


static bool          gv_thr_rs_init = false;
static Thr_Rs_State  gv_thr_rs_state;

//
// forwards
//
static void  thr_int_rs_pipe_init(Thr_Rs_Pipe *pp_pipe,
                                  const char   *pp_desc);
static void  thr_int_rs_sig_action_suspend();
static void  thr_int_rs_sig_resume_ack(Thr_Rs_Pipe *pp_pipe);
static void  thr_int_rs_sig_suspend_ack(Thr_Rs_Pipe *pp_pipe);
static void  thr_int_rs_sig_suspend_wait(Thr_Rs_Pipe *pp_pipe);

static int thr_int_rs_init() {
    gv_thr_rs_state.iv_state = THR_RS_RESUME;
    gv_thr_rs_state.iv_inx = 0;
    thr_int_rs_pipe_init(&gv_thr_rs_state.iv_m2s_pipe, "m->s");
    thr_int_rs_pipe_init(&gv_thr_rs_state.iv_s2m_pipe, "s->m");
    return true;
}

static void thr_int_rs_pipe_init(Thr_Rs_Pipe *pp_pipe, const char *pp_desc) {
    const char *WHERE = "thr_int_rs_pipe_init";
    int         lv_err;

    pp_pipe->ip_desc = pp_desc;
    pp_pipe->ia_pipe[THR_RS_PIPE_READ] = 0;  // int
    pp_pipe->ia_pipe[THR_RS_PIPE_WRITE] = 0; // int
    lv_err = pipe(pp_pipe->ia_pipe);
    SB_util_assert_ieq(lv_err, 0);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "fd=%d(r)/%d(w)\n",
                           pp_pipe->ia_pipe[THR_RS_PIPE_READ],
                           pp_pipe->ia_pipe[THR_RS_PIPE_WRITE]);
}

static void thr_int_rs_pipe_read(const char    *pp_where,
                                 Thr_Rs_Action  pv_action,
                                 Thr_Rs_Pipe   *pp_pipe,
                                 int            pv_cnt) {
    const char *WHERE = "thr_int_rs_pipe_read";
    char        la_buf[THR_RS_MAX_THREADS];
    char        la_errno[100];
    int         lv_fd;
    int         lv_inx;
    ssize_t     lv_rerr;
    int         lv_rerri;

    SB_util_assert_ile(pv_cnt, THR_RS_MAX_THREADS);
    lv_fd = pp_pipe->ia_pipe[THR_RS_PIPE_READ];
    do {
        if (gv_ms_trace && gv_ms_trace_verbose)
            trace_where_printf(WHERE, "%s(%s) reading %d char(s) from fd=%d\n",
                               pp_where, pp_pipe->ip_desc, pv_cnt, lv_fd);
        lv_rerr = read(lv_fd, la_buf, pv_cnt);
        lv_rerri = static_cast<int>(lv_rerr);
        if (gv_ms_trace && gv_ms_trace_verbose) {
            if (lv_rerr == -1)
                trace_where_printf(WHERE, "%s(%s) read returned errno=%d(%s), fd=%d\n",
                                   pp_where,
                                   pp_pipe->ip_desc,
                                   errno,
                                   strerror_r(errno,
                                              la_errno,
                                              sizeof(la_errno)),
                                   lv_fd);
            else {
                trace_where_printf(WHERE, "%s(%s) read returned err=" PFSSIZE ", fd=%d\n",
                                   pp_where, pp_pipe->ip_desc, lv_rerr, lv_fd);
                trace_print_data(la_buf, lv_rerri, lv_rerri);
            }
        }
        pp_pipe->ia_errno[THR_RS_PIPE_READ] = errno;
        SB_util_assert_stne(lv_rerr, -1);
        for (lv_inx = 0; lv_inx < lv_rerr; lv_inx++)
            SB_util_assert_ieq(la_buf[lv_inx], pv_action);
        pv_action = pv_action; // touch (in case assert disabled)
        pv_cnt -= lv_rerri;
    } while (pv_cnt > 0);
}

static void thr_int_rs_pipe_write(const char    *pp_where,
                                  Thr_Rs_Action  pv_action,
                                  Thr_Rs_Pipe   *pp_pipe,
                                  int            pv_cnt) {
    const char *WHERE = "thr_int_rs_pipe_write";
    char        la_buf[THR_RS_MAX_THREADS];
    char        la_errno[100];
    int         lv_fd;
    int         lv_inx;
    ssize_t     lv_werr;

    SB_util_assert_ile(pv_cnt, THR_RS_MAX_THREADS);
    for (lv_inx = 0; lv_inx < pv_cnt; lv_inx++)
        la_buf[lv_inx] = pv_action;
    lv_fd = pp_pipe->ia_pipe[THR_RS_PIPE_WRITE];
    do {
        if (gv_ms_trace && gv_ms_trace_verbose)
            trace_where_printf(WHERE, "%s(%s) writing %d char(s) to fd=%d\n",
                               pp_where, pp_pipe->ip_desc, pv_cnt, lv_fd);
        lv_werr = write(lv_fd, la_buf, pv_cnt);
        if (gv_ms_trace && gv_ms_trace_verbose) {
            if (lv_werr == -1)
                trace_where_printf(WHERE, "%s(%s) write returned errno=%d(%s), fd=%d\n",
                                   pp_where,
                                   pp_pipe->ip_desc,
                                   errno,
                                   strerror_r(errno,
                                              la_errno,
                                              sizeof(la_errno)),
                                   lv_fd);
            else
                trace_where_printf(WHERE, "%s(%s) write returned err=" PFSSIZE ", fd=%d\n",
                                   pp_where, pp_pipe->ip_desc, lv_werr, lv_fd);
        }
        pp_pipe->ia_errno[THR_RS_PIPE_WRITE] = errno;
        SB_util_assert_stne(lv_werr, -1);
        pv_cnt -= pv_cnt;
    } while (pv_cnt > 0);
}

static void thr_int_rs_sig(int, siginfo_t *, void *) {
    thr_int_rs_sig_action_suspend();
}

// handle action-suspend
static void thr_int_rs_sig_action_suspend() {
    // enter here with suspend action
    thr_int_rs_sig_suspend_ack(&gv_thr_rs_state.iv_s2m_pipe);

    // stall here until the pipe is written to
    thr_int_rs_sig_suspend_wait(&gv_thr_rs_state.iv_m2s_pipe);

    // pipe written to - send resume ack
    thr_int_rs_sig_resume_ack(&gv_thr_rs_state.iv_s2m_pipe);
}

// this is for stack trace!
static void thr_int_rs_sig_resume_ack(Thr_Rs_Pipe *pp_pipe) {
    thr_int_rs_pipe_write("resume-ack", THR_RS_RESUME, pp_pipe, 1);
}

// this is for stack trace!
static void thr_int_rs_sig_suspend_ack(Thr_Rs_Pipe *pp_pipe) {
    thr_int_rs_pipe_write("suspend-ack", THR_RS_SUSPEND, pp_pipe, 1);
}

// this is for stack trace!
static void thr_int_rs_sig_suspend_wait(Thr_Rs_Pipe *pp_pipe) {
    thr_int_rs_pipe_read("suspend-wait", THR_RS_RESUME, pp_pipe, 1);
}

// handle resume
static void thr_int_rs_threads_action_resume(const char *pp_where) {
    int lv_cnt_max;
    int lv_inx;

    lv_cnt_max = gv_thr_rs_state.iv_inx - 1;

    if (gv_ms_trace)
        trace_where_printf(pp_where, "send resume count=%d\n", lv_cnt_max);
    thr_int_rs_pipe_write("resume",
                          THR_RS_RESUME,
                          &gv_thr_rs_state.iv_m2s_pipe,
                          lv_cnt_max);

    // could read all acks at once - but this allows showing progress
    for (lv_inx = 0; lv_inx < lv_cnt_max; lv_inx++) {
        if (gv_ms_trace)
            trace_where_printf(pp_where, "wait resume ack count=%d/%d\n",
                               lv_inx + 1, lv_cnt_max);
        thr_int_rs_pipe_read("resume-ack",
                             THR_RS_RESUME,
                             &gv_thr_rs_state.iv_s2m_pipe,
                             1);
        if (gv_ms_trace)
            trace_where_printf(pp_where, "wait resume acked count=%d/%d\n",
                               lv_inx + 1, lv_cnt_max);
    }

    // switch state
    gv_thr_rs_state.iv_state = THR_RS_RESUME;
}

// handle suspend
static void thr_int_rs_threads_action_suspend(const char *pp_where) {
    Thr_Rs_Thread *lp_thr;
    int            lv_cnt;
    int            lv_cnt_max;
    long           lv_errl;
    int            lv_inx;
    int            lv_tid;
    int            lv_tid_self;

    lv_tid_self = gettid();
    lv_cnt = 1;
    lv_cnt_max = gv_thr_rs_state.iv_inx - 1;
    for (lv_inx = 0; lv_inx < gv_thr_rs_state.iv_inx; lv_inx++) {
        lp_thr = &gv_thr_rs_state.ia_threads[lv_inx];
        lv_tid = lp_thr->iv_tid;
        if (lv_tid != lv_tid_self) {
            if (gv_ms_trace)
                trace_where_printf(pp_where, "send signal count=%d/%d, tid=%d\n",
                                   lv_cnt, lv_cnt_max, lv_tid);

            lv_errl = tgkill(getpid(), lv_tid, SIGURG);
            SB_util_assert_leq(lv_errl, 0);

            if (gv_ms_trace)
                trace_where_printf(pp_where, "wait suspend ack count=%d/%d, tid=%d\n",
                                   lv_cnt, lv_cnt_max, lv_tid);
            thr_int_rs_pipe_read("suspend-ack",
                                 THR_RS_SUSPEND,
                                 &gv_thr_rs_state.iv_s2m_pipe,
                                 1);
            if (gv_ms_trace)
                trace_where_printf(pp_where, "wait suspend acked count=%d/%d, tid=%d\n",
                                   lv_cnt, lv_cnt_max, lv_tid);
            lv_cnt++;
        }
    }

    // switch state
    gv_thr_rs_state.iv_state = THR_RS_SUSPEND;
}

static bool thr_int_rs_threads_init() {
    struct sigaction lv_act;
    int              lv_err;

    // setup signal handler
    memset(&lv_act, 0, sizeof(struct sigaction));
    lv_act.sa_sigaction = thr_int_rs_sig;
    // SA_SIGINFO says to use sa_sigaction
    // SA_NODEFER says to allow signal stacking
    lv_act.sa_flags = SA_SIGINFO | SA_NODEFER;
    lv_err = sigaction(SIGURG, &lv_act, NULL);
    SB_util_assert_ieq(lv_err, 0);

    return true;
}

static unsigned long long thr_int_rs_threads_init_setup_get_blocked_sig(int pv_tid) {
    char                la_line[BUFSIZ];
    char                la_status[100];
    char               *lp_s;
    FILE               *lp_status;
    unsigned long long  lv_sigblk;

    sprintf(la_status, "/proc/%d/status", pv_tid);
    lp_status = fopen(la_status, "r");
    lv_sigblk = 0;
    if (lp_status != NULL) {
        for (;;) {
            lp_s = fgets(la_line, sizeof(la_line), lp_status);
            if (lp_s == NULL)
                break;
            if (memcmp(la_line, "SigBlk:\t", 8) == 0) {
                sscanf(la_line, "SigBlk:\t%llx", &lv_sigblk);
            }
        }
        fclose(lp_status);
    }
    return lv_sigblk;
}

static void thr_int_rs_threads_init_setup_thr(const char *pp_where) {
    DIR                *lp_dir;
    struct dirent      *lp_dirent;
    Thr_Rs_Thread      *lp_thr;
    struct dirent       lv_entry;
    int                 lv_err;
    unsigned long long  lv_sigblk_blocked;
    unsigned long long  lv_sigblk_tid;
    sigset_t            lv_sigset_new;
    sigset_t            lv_sigset_old;
    int                 lv_tid;

    //
    // get blocked-signals value
    // (for check against a thread blocking all signals)
    //
    lv_err = sigfillset(&lv_sigset_new);
    SB_util_assert_ieq(lv_err, 0);
    lv_err = pthread_sigmask(SIG_SETMASK, &lv_sigset_new, &lv_sigset_old);
    SB_util_assert_ieq(lv_err, 0);
    lv_sigblk_blocked = thr_int_rs_threads_init_setup_get_blocked_sig(gettid());
    lv_err = pthread_sigmask(SIG_SETMASK, &lv_sigset_old, NULL);
    SB_util_assert_ieq(lv_err, 0);

    gv_thr_rs_state.iv_inx = 0;
    lp_dir = opendir("/proc/self/task");
    for (;;) {
        if (lp_dir == NULL)
            break;
        lv_err = readdir_r(lp_dir, &lv_entry, &lp_dirent);
        if (lv_err)
            break;
        if (lp_dirent == NULL)
            break;
        if (lp_dirent->d_ino == 0) // invalid inode-number
            continue;
        if (lp_dirent->d_name[0] == '.') // relative
            continue;
        sscanf(lp_dirent->d_name, "%d", &lv_tid);

        // if thread has all signals blocked - don't include
        lv_sigblk_tid = thr_int_rs_threads_init_setup_get_blocked_sig(lv_tid);
        if (lv_sigblk_tid == lv_sigblk_blocked) {
            if (gv_ms_trace_params)
                trace_where_printf(pp_where,
                                   "tid=%d has all signals blocked - skipping\n",
                                   gettid());
            continue;
        }

        SB_util_assert_ilt(gv_thr_rs_state.iv_inx, THR_RS_MAX_THREADS);
        lp_thr = &gv_thr_rs_state.ia_threads[gv_thr_rs_state.iv_inx];
        lp_thr->iv_tid = lv_tid;
        gv_thr_rs_state.iv_inx++;
    }
    if (lp_dir != NULL)
        closedir(lp_dir);
}

//
// Resume all suspended-threads
//
SB_Export int thread_resume_suspended() {
    const char *WHERE = "thread_resume_suspended";
    short       lv_fserr;

    if (!gv_thr_rs_init)
        gv_thr_rs_init = thr_int_rs_init();
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    if (gv_thr_rs_state.iv_state == THR_RS_SUSPEND) {
        lv_fserr = XZFIL_ERR_OK;
        thr_int_rs_threads_action_resume(WHERE);
    } else {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "already resume'd\n");
        lv_fserr = XZFIL_ERR_INVALIDSTATE;
    }
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Suspend all threads-except self
//
SB_Export int thread_suspend_all() {
    const char *WHERE = "thread_suspend_all";
    short       lv_fserr;
    static bool lv_inited = false;

    if (!gv_thr_rs_init)
        gv_thr_rs_init = thr_int_rs_init();
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    if (gv_thr_rs_state.iv_state == THR_RS_RESUME) {
        lv_fserr = XZFIL_ERR_OK;
        if (!lv_inited)
            lv_inited = !thr_int_rs_threads_init();
        thr_int_rs_threads_init_setup_thr(WHERE);
        thr_int_rs_threads_action_suspend(WHERE);
    } else {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "already suspend'd\n");
        lv_fserr = XZFIL_ERR_INVALIDSTATE;
    }
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}
