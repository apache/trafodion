///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

//
// suspend/resume threads
//

#ifdef USE_FORK_SUSPEND_RESUME

//
// strategy...
//    create m2s - master-to-slave pipe_ptr
//    create s2m - slave-to-master pipe_ptr
//    where master is thread doing the suspend/resume
//    where slave are thread(s) getting suspended/resumed
//
// thread_suspend_all -
//   for (each-thread-except-self)
//   {
//       tgkill(thread, SIGURG); // tell thread to suspend
//       pipe_read(s2m);         // wait for ack
//   }
//
// thread_resume_suspended -
//   for (each-thread-except-self)
//   {
//       pipe_write(m2s); // tell thread to resume
//       pipe_read(s2m);  // wait for ack
//   }
//
//   At each-thread-except-self:
//     will take mon_int_rs_sig, which will then:
//       // must do in async-safe manner
//       pipe_write(s2m);  // ack-suspend
//       pipe_read(m2s);   // wait for resume
//       pipe_write(s2m);  // ack-resume
//

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <linux/unistd.h>
#include <sys/stat.h>

#include "montrace.h"

#if __WORDSIZE == 64
#  define MON_RS_PFSSIZE  "%ld"
#else
#  define MON_RS_PFSSIZE  "%u"
#endif

#define gettid() syscall(__NR_gettid)
#define tgkill(pid,tid,sig) syscall(__NR_tgkill,pid,tid,sig)


enum { MON_RS_MAX_THREADS = 10 };

enum { MON_RS_PIPE_READ   = 0 };
enum { MON_RS_PIPE_WRITE  = 1 };

//
// pipe_ptr
//
typedef struct Mon_Rs_Pipe
{
    int         Errno[2];
    int         Pipe[2];
    const char *Desc;
} Mon_Rs_Pipe;

//
// action node
//
typedef enum
{
    MON_RS_RESUME  = 1,
    MON_RS_SUSPEND = 2
} Mon_Rs_Action;

//
// thread node
//
typedef struct Mon_Rs_Thread
{
    int Tid;
} Mon_Rs_Thread;

//
// state
//
typedef struct Mon_Rs_State
{
    int            Inx;
    Mon_Rs_Action  State;
    Mon_Rs_Thread  Threads[MON_RS_MAX_THREADS];
    Mon_Rs_Pipe    M2s_pipe;
    Mon_Rs_Pipe    S2m_pipe;
} Mon_Rs_State;


static bool          Mon_rs_init = false;
static Mon_Rs_State  Mon_rs_state;

//
// forwards
//
static void  mon_int_rs_pipe_init(Mon_Rs_Pipe *pipe_ptr,
                                  const char  *desc_ptr);
static void  mon_int_rs_sig_action_suspend();
static void  mon_int_rs_sig_resume_ack(Mon_Rs_Pipe *pipe_ptr);
static void  mon_int_rs_sig_suspend_ack(Mon_Rs_Pipe *pipe_ptr);
static void  mon_int_rs_sig_suspend_wait(Mon_Rs_Pipe *pipe_ptr);

static int mon_int_rs_init()
{
    Mon_rs_state.State = MON_RS_RESUME;
    Mon_rs_state.Inx = 0;
    mon_int_rs_pipe_init(&Mon_rs_state.M2s_pipe, "m->s");
    mon_int_rs_pipe_init(&Mon_rs_state.S2m_pipe, "s->m");
    return true;
}

static void mon_int_rs_pipe_init(Mon_Rs_Pipe *pipe_ptr, const char *desc_ptr)
{
    const char *WHERE = "mon_int_rs_pipe_init";
    int         err;

    pipe_ptr->Desc = desc_ptr;
    pipe_ptr->Pipe[MON_RS_PIPE_READ] = 0;
    pipe_ptr->Pipe[MON_RS_PIPE_WRITE] = 0;
    err = pipe(pipe_ptr->Pipe);
    assert(err == 0);
    if (trace_settings & TRACE_PROCESS)
        trace_where_printf(WHERE, "fd=%d(r)/%d(w)\n",
                           pipe_ptr->Pipe[MON_RS_PIPE_READ],
                           pipe_ptr->Pipe[MON_RS_PIPE_WRITE]);
}

static void mon_int_rs_pipe_read(const char    *where_ptr,
                                 Mon_Rs_Action  action,
                                 Mon_Rs_Pipe   *pipe_ptr,
                                 int            cnt)
{
    const char *WHERE = "mon_int_rs_pipe_read";
    char        buf[MON_RS_MAX_THREADS];
    int         fd;
    int         inx;
    ssize_t     rerr;

    assert(cnt <= MON_RS_MAX_THREADS);
    fd = pipe_ptr->Pipe[MON_RS_PIPE_READ];
    do
    {
        if (trace_settings & TRACE_PROCESS_DETAIL)
            trace_where_printf(WHERE, "%s(%s) reading %d char(s) from fd=%d\n",
                               where_ptr, pipe_ptr->Desc, cnt, fd);
        rerr = read(fd, buf, cnt);
        if (trace_settings & TRACE_PROCESS_DETAIL)
        {
            if (rerr == -1)
                trace_where_printf(WHERE, "%s(%s) read returned errno=%d(%s), fd=%d\n",
                                   where_ptr,
                                   pipe_ptr->Desc,
                                   errno,
                                   strerror(errno),
                                   fd);
            else
            {
                trace_where_printf(WHERE, "%s(%s) read returned err=" MON_RS_PFSSIZE ", fd=%d\n",
                                   where_ptr, pipe_ptr->Desc, rerr, fd);
                trace_print_data(buf, rerr, rerr);
            }
        }
        pipe_ptr->Errno[MON_RS_PIPE_READ] = errno;
        assert(rerr != -1);
        for (inx = 0; inx < rerr; inx++)
            assert(buf[inx] == action);
        cnt -= rerr;
    } while (cnt > 0);
}

static void mon_int_rs_pipe_write(const char    *where_ptr,
                                  Mon_Rs_Action  action,
                                  Mon_Rs_Pipe   *pipe_ptr,
                                  int            cnt)
{
    const char *WHERE = "mon_int_rs_pipe_write";
    char        buf[MON_RS_MAX_THREADS];
    int         fd;
    int         inx;
    ssize_t     werr;

    assert(cnt <= MON_RS_MAX_THREADS);
    for (inx = 0; inx < cnt; inx++)
        buf[inx] = action;
    fd = pipe_ptr->Pipe[MON_RS_PIPE_WRITE];
    do
    {
        if (trace_settings & TRACE_PROCESS_DETAIL)
            trace_where_printf(WHERE, "%s(%s) writing %d char(s) to fd=%d\n",
                               where_ptr, pipe_ptr->Desc, cnt, fd);
        werr = write(fd, buf, cnt);
        if (trace_settings & TRACE_PROCESS_DETAIL)
        {
            if (werr == -1)
                trace_where_printf(WHERE, "%s(%s) write returned errno=%d(%s), fd=%d\n",
                                   where_ptr,
                                   pipe_ptr->Desc,
                                   errno,
                                   strerror(errno),
                                   fd);
            else
                trace_where_printf(WHERE, "%s(%s) write returned err=" MON_RS_PFSSIZE ", fd=%d\n",
                                   where_ptr, pipe_ptr->Desc, werr, fd);
        }
        pipe_ptr->Errno[MON_RS_PIPE_WRITE] = errno;
        assert(werr != -1);
        cnt -= cnt;
    } while (cnt > 0);
}

static void mon_int_rs_sig(int, siginfo_t *, void *)
{
    mon_int_rs_sig_action_suspend();
}

// handle action-suspend
static void mon_int_rs_sig_action_suspend()
{
    // enter here with suspend action
    mon_int_rs_sig_suspend_ack(&Mon_rs_state.S2m_pipe);

    // stall here until the pipe_ptr is written to
    mon_int_rs_sig_suspend_wait(&Mon_rs_state.M2s_pipe);

    // pipe_ptr written to - send resume ack
    mon_int_rs_sig_resume_ack(&Mon_rs_state.S2m_pipe);
}

// this is for stack trace!
static void mon_int_rs_sig_resume_ack(Mon_Rs_Pipe *pipe_ptr)
{
    mon_int_rs_pipe_write("resume-ack", MON_RS_RESUME, pipe_ptr, 1);
}

// this is for stack trace!
static void mon_int_rs_sig_suspend_ack(Mon_Rs_Pipe *pipe_ptr)
{
    mon_int_rs_pipe_write("suspend-ack", MON_RS_SUSPEND, pipe_ptr, 1);
}

// this is for stack trace!
static void mon_int_rs_sig_suspend_wait(Mon_Rs_Pipe *pipe_ptr)
{
    mon_int_rs_pipe_read("suspend-wait", MON_RS_RESUME, pipe_ptr, 1);
}

// handle resume
static void mon_int_rs_threads_action_resume(const char *where_ptr)
{
    int cnt_max;
    int inx;

    cnt_max = Mon_rs_state.Inx - 1;

    if (trace_settings & TRACE_PROCESS)
        trace_where_printf(where_ptr, "send resume count=%d\n", cnt_max);
    mon_int_rs_pipe_write("resume",
                          MON_RS_RESUME,
                          &Mon_rs_state.M2s_pipe,
                          cnt_max);

    // could read all acks at once - but this allows showing progress
    for (inx = 0; inx < cnt_max; inx++)
    {
        if (trace_settings & TRACE_PROCESS)
            trace_where_printf(where_ptr, "wait resume ack count=%d/%d\n",
                               inx + 1, cnt_max);
        mon_int_rs_pipe_read("resume-ack",
                             MON_RS_RESUME,
                             &Mon_rs_state.S2m_pipe,
                             1);
        if (trace_settings & TRACE_PROCESS)
            trace_where_printf(where_ptr, "wait resume acked count=%d/%d\n",
                               inx + 1, cnt_max);
    }

    // switch state
    Mon_rs_state.State = MON_RS_RESUME;
}

// handle suspend
static void mon_int_rs_threads_action_suspend(const char *where_ptr)
{
    int            cnt;
    int            cnt_max;
    long           errl;
    int            inx;
    Mon_Rs_Thread *thr_ptr;
    int            tid;
    int            tid_self;

    tid_self = static_cast<int>(gettid());
    cnt = 1;
    cnt_max = Mon_rs_state.Inx - 1;
    for (inx = 0; inx < Mon_rs_state.Inx; inx++)
    {
        thr_ptr = &Mon_rs_state.Threads[inx];
        tid = thr_ptr->Tid;
        if (tid != tid_self)
        {
            if (trace_settings & TRACE_PROCESS)
                trace_where_printf(where_ptr, "send signal count=%d/%d, tid=%d\n",
                                   cnt, cnt_max, tid);

            errl = tgkill(getpid(), tid, SIGURG);
            assert(errl == 0);

            if (trace_settings & TRACE_PROCESS)
                trace_where_printf(where_ptr, "wait suspend ack count=%d/%d, tid=%d\n",
                                   cnt, cnt_max, tid);
            mon_int_rs_pipe_read("suspend-ack",
                                 MON_RS_SUSPEND,
                                 &Mon_rs_state.S2m_pipe,
                                 1);
            if (trace_settings & TRACE_PROCESS)
                trace_where_printf(where_ptr, "wait suspend acked count=%d/%d, tid=%d\n",
                                   cnt, cnt_max, tid);
            cnt++;
        }
    }

    // switch state
    Mon_rs_state.State = MON_RS_SUSPEND;
}

static bool mon_int_rs_threads_init()
{
    struct sigaction act;
    int              err;

    // setup signal handler
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = mon_int_rs_sig;
    // SA_SIGINFO says to use sa_sigaction
    // SA_NODEFER says to allow signal stacking
    act.sa_flags = SA_SIGINFO | SA_NODEFER;
    err = sigaction(SIGURG, &act, NULL);
    assert(err == 0);

    return true;
}

static void mon_int_rs_threads_init_setup_thr()
{
    DIR           *dir_ptr;
    struct dirent *dirent_ptr;
    Mon_Rs_Thread *thr_ptr;
    int            tid;

    Mon_rs_state.Inx = 0;
    dir_ptr = opendir("/proc/self/task");
    for (;;)
    {
        if (dir_ptr == NULL)
            break;
        dirent_ptr = readdir(dir_ptr);
        if (dirent_ptr == NULL)
            break;
        if (dirent_ptr->d_ino == 0) // invalid inode-number
            continue;
        if (dirent_ptr->d_name[0] == '.') // relative
            continue;
        sscanf(dirent_ptr->d_name, "%d", &tid);
        assert(Mon_rs_state.Inx < MON_RS_MAX_THREADS);
        thr_ptr = &Mon_rs_state.Threads[Mon_rs_state.Inx];
        thr_ptr->Tid = tid;
        Mon_rs_state.Inx++;
    }
    if (dir_ptr != NULL)
        closedir(dir_ptr);
}

//
// debug
//
void mon_thread_debug(const char *who, const char *fname)
{
    char         buf[500];
    int          err;
    char        *p;
    struct stat  statv;

    err = stat(fname, &statv);
    if (err == -1)
    {
        p = getcwd(buf, sizeof(buf));
        printf("%s: create file %s/%s to continue\n",
               who, p, fname);
        printf("%s: slave pid=%d\n", who, getpid());
        for (;;)
        {
            err = stat(fname, &statv);
            if (err == 0)
            {
                printf("%s: %s detected - continuing\n", who, fname);
                break;
            }
            sleep(1);
        }
    }
}

//
// Resume all suspended-threads
//
bool mon_thread_resume_suspended()
{
    const char *WHERE = "mon_thread_resume_suspended";
    bool        ret;

    if (!Mon_rs_init)
        Mon_rs_init = mon_int_rs_init();
    if (trace_settings & TRACE_PROCESS)
        trace_where_printf(WHERE, "ENTER\n");
    if (Mon_rs_state.State == MON_RS_SUSPEND)
    {
        ret = true;
        mon_int_rs_threads_action_resume(WHERE);
    } else
    {
        if (trace_settings & TRACE_PROCESS)
            trace_where_printf(WHERE, "already resume'd\n");
        ret = false;
    }
    if (trace_settings & TRACE_PROCESS)
        trace_where_printf(WHERE, "EXIT ret=%d\n", ret);
    return ret;
}

//
// Suspend all threads-except self
//
bool mon_thread_suspend_all()
{
    const char *WHERE = "mon_thread_suspend_all";
    static bool inited = false;
    bool        ret;

    if (!Mon_rs_init)
        Mon_rs_init = mon_int_rs_init();
    if (trace_settings & TRACE_PROCESS)
        trace_where_printf(WHERE, "ENTER\n");
    if (Mon_rs_state.State == MON_RS_RESUME)
    {
        ret = true;
        if (!inited)
            inited = !mon_int_rs_threads_init();
        mon_int_rs_threads_init_setup_thr();
        mon_int_rs_threads_action_suspend(WHERE);
    } else
    {
        if (trace_settings & TRACE_PROCESS)
            trace_where_printf(WHERE, "already suspend'd\n");
        ret = false;
    }
    if (trace_settings & TRACE_PROCESS)
        trace_where_printf(WHERE, "EXIT ret=%d\n", ret);
    return ret;
}

#endif // USE_FORK_SUSPEND_RESUME
