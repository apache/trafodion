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
//   SB_THREAD_PRINT_THREAD_CALLS
//

//#define SB_THREAD_PRINT_THREAD_CALLS

//
// Implement SB_Thread class
//
#include <queue>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <linux/unistd.h> // gettid

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

#include "seabed/thread.h"
#include "seabed/trace.h"

#include "util.h"
#include "utracex.h"

// externs
extern bool gv_sb_trace_thread;


// ----------------------------------------------------------------------------

SB_Thread::Preemptive_Mutex::Preemptive_Mutex() {
}

SB_Thread::Preemptive_Mutex::~Preemptive_Mutex() {
}

int SB_Thread::Preemptive_Mutex::lock() {
    return Mutex::lock();
}

int SB_Thread::Preemptive_Mutex::trylock() {
    return Mutex::trylock();
}

int SB_Thread::Preemptive_Mutex::unlock() {
    return Mutex::unlock();
}

// ----------------------------------------------------------------------------

SB_Thread::Scoped_Mutex::Scoped_Mutex(Mutex &pr_mutex) :
    ir_mutex(pr_mutex), iv_lock(true) {
    int lv_ret = ir_mutex.lock();
    SB_util_assert_ieq(lv_ret, 0); // sw fault
    lv_ret = lv_ret; // touch (in case assert disabled)
}

SB_Thread::Scoped_Mutex::~Scoped_Mutex() {
    if (iv_lock) {
        int lv_ret = ir_mutex.unlock();
        SB_util_assert_ieq(lv_ret, 0); // sw fault
        lv_ret = lv_ret; // touch (in case assert disabled)
    }
}

void SB_Thread::Scoped_Mutex::lock() {
    if (!iv_lock) {
        int lv_ret = ir_mutex.lock();
        SB_util_assert_ieq(lv_ret, 0); // sw fault
        lv_ret = lv_ret; // touch (in case assert disabled)
        iv_lock = true;
    }
}

void SB_Thread::Scoped_Mutex::unlock() {
    if (iv_lock) {
        int lv_ret = ir_mutex.unlock();
        SB_util_assert_ieq(lv_ret, 0); // sw fault
        lv_ret = lv_ret; // touch (in case assert disabled)
        iv_lock = false;
    }
}

// ----------------------------------------------------------------------------

// resume/suspend context
typedef std::pair<int, SB_Thread::Thread *> Rs_Type;
static bool                                 gv_rs_inited = false;
static std::queue<Rs_Type>                  gv_rs_queue;
static sigset_t                             gv_rs_sigset;
enum { RS_RESUME, RS_SUSPEND };

// resume/suspend signal-handler
static void sig_rs(int, siginfo_t *, void *) {
    int lv_status;

    if (!gv_rs_queue.empty()) {
        Rs_Type lv_front = gv_rs_queue.front();
        gv_rs_queue.pop();
        int lv_action = lv_front.first;
        SB_Thread::Thread *lp_thr = lv_front.second;
        SB_Thread::CV *lp_cv = reinterpret_cast<SB_Thread::CV *>(lp_thr->ip_rs);
        if (lv_action == RS_SUSPEND) {
            lv_status = lp_cv->wait(true); // need lock
            SB_util_assert_ieq(lv_status, 0);
        } else {
            // strict signal() does not seem to always work
            lv_status = lp_cv->broadcast(true); // need lock
            SB_util_assert_ieq(lv_status, 0);
        }
    }
}

int SB_Thread::Thread::cv_count = 1;

SB_Thread::Thread::Thread(Sthr::Function pv_fun, const char *pp_name)
: ip_tid(0), ip_id(NULL), iv_daemon(false), iv_del_exit(true), iv_fun(pv_fun) {
    char la_name[100];

#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::Thread(%p,%s) ENTRY\n",
                 static_cast<void *>(&pv_fun), pp_name);
#endif
    if (pp_name == NULL) {
        sprintf(la_name, "thr%d", (cv_count++));
        pp_name = la_name;
    }
    ip_rs = reinterpret_cast<Thr_Gen_Ptr>(new CV());
    ip_name = new char[strlen(pp_name)+1];
    strcpy(ip_name, pp_name);
    if (!gv_rs_inited) {
        gv_rs_inited = true;
        sigemptyset(&gv_rs_sigset);
        sigaddset(&gv_rs_sigset, SIGUSR1);
    }
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::Thread() EXIT, name=%s\n", ip_name);
#endif
}

SB_Thread::Thread::~Thread() {
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::~Thread() ENTRY, name=%s\n", ip_name);
#endif
    CV *lp_cv = reinterpret_cast<CV *>(ip_rs);
    bool lv_del_exit = iv_del_exit;
    delete lp_cv;
    delete [] ip_name;
    Sthr::delete_id(ip_id);
    if (lv_del_exit)
        if (static_cast<pid_t>(reinterpret_cast<long>(ip_tid)) == gettid())
            Sthr::exit(NULL);
}

void SB_Thread::Thread::allow_suspend() {
    struct sigaction lv_act;
    memset(&lv_act, 0, sizeof(struct sigaction));
    lv_act.sa_sigaction = sig_rs;
    // SA_SIGINFO says to use sa_sigaction
    // SA_NODEFER says to allow signal stacking
    lv_act.sa_flags = SA_SIGINFO | SA_NODEFER;
    int lv_rc = sigaction(SIGUSR1, &lv_act, NULL);
    SB_util_assert_ieq(lv_rc, 0);
    lv_rc = lv_rc; // touch (in case assert disabled)
}

void SB_Thread::Thread::delete_exit(bool pv_del_exit) {
    iv_del_exit = pv_del_exit;
}

void *SB_Thread::Thread::disp(void *pp_arg) {
    const char *WHERE = "Thread::start(via)";
    char        la_name[100];
    void       *lp_fun;
    void       *lp_ret;

    lp_fun = SB_CB_TO_PTR(iv_fun);
    if (gv_sb_trace_thread) {
        la_name[sizeof(la_name) - 1] = '\0';
        strncpy(la_name, ip_name, sizeof(la_name) - 1);
        trace_where_printf(WHERE, "thread started name=%s, fun=%p, arg=%p\n",
                           la_name, lp_fun, pp_arg);
    }
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_THREAD_START_DISP1,
                       gettid(),
                       reinterpret_cast<SB_Utrace_API_Info_Type>(lp_fun));
    lp_ret = iv_fun(pp_arg);
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_THREAD_EXIT_DISP1,
                       gettid(),
                       reinterpret_cast<SB_Utrace_API_Info_Type>(lp_fun));
    if (gv_sb_trace_thread)
        trace_where_printf(WHERE, "thread exiting name=%s, fun=%p, arg=%p, ret=%p\n",
                           la_name, lp_fun, pp_arg, lp_ret);
    return lp_ret;
}

void SB_Thread::Thread::exit(void *pp_value) {
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::exit(%p) ENTRY, name=%s\n", pp_value, ip_name);
#endif
    delete this;
    Sthr::exit(pp_value);
}

char *SB_Thread::Thread::get_name() {
    return ip_name;
}

int SB_Thread::Thread::join(void **ppp_result) {
    int lv_ret;
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::join(%p) ENTRY, name=%s\n",
                 static_cast<void *>(ppp_result), ip_name);
#endif
    if (ip_id == NULL)
        lv_ret = ESRCH;
    else
        lv_ret = Sthr::join(ip_id, ppp_result);
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::join() EXIT, name=%s, ret=%d\n", ip_name, lv_ret);
#endif
    return lv_ret;
}

int SB_Thread::Thread::kill(int pv_sig) {
    int lv_ret;
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::kill(%d) ENTRY, name=%s\n",
                 pv_sig, ip_name);
#endif
    if (ip_id == NULL)
        lv_ret = ESRCH;
    else
        lv_ret = Sthr::kill(ip_id, pv_sig);
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::kill() EXIT, name=%s, ret=%d\n", ip_name, lv_ret);
#endif
    return lv_ret;
}

void SB_Thread::Thread::resume() {
    int lv_rc = pthread_sigmask(SIG_BLOCK, &gv_rs_sigset, NULL);
    SB_util_assert_ine(lv_rc, -1);
    gv_rs_queue.push(std::make_pair(static_cast<int>(RS_RESUME), this));
    lv_rc = kill(SIGUSR1);
    SB_util_assert_ieq(lv_rc, 0);
    lv_rc = pthread_sigmask(SIG_UNBLOCK, &gv_rs_sigset, NULL);
    SB_util_assert_ine(lv_rc, -1);
}

void SB_Thread::Thread::set_daemon(bool pv_daemon) {
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::set_daemon(%d) ENTRY, name=%s\n", pv_daemon, ip_name);
#endif
    iv_daemon = pv_daemon;
}

void SB_Thread::Thread::sleep(int pv_ms) {
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::sleep(%d) ENTRY, name=%s\n", pv_ms, ip_name);
#endif
    Sthr::sleep(pv_ms);
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::sleep() EXIT, name=%s\n", ip_name);
#endif
}

void SB_Thread::Thread::suspend() {
    int lv_rc = pthread_sigmask(SIG_BLOCK, &gv_rs_sigset, NULL);
    SB_util_assert_ine(lv_rc, -1);
    gv_rs_queue.push(std::make_pair(static_cast<int>(RS_SUSPEND), this));
    lv_rc = kill(SIGUSR1);
    SB_util_assert_ieq(lv_rc, 0);
    lv_rc = pthread_sigmask(SIG_UNBLOCK, &gv_rs_sigset, NULL);
    SB_util_assert_ine(lv_rc, -1);
}

static void *thread_fun(void *pp_arg) {
    SB_Thread::Thread *lp_thread = static_cast<SB_Thread::Thread *>(pp_arg);

    SB_Thread::Sthr::set_name(lp_thread->get_name());
    lp_thread->ip_tid = reinterpret_cast<SB_Thread::Thr_Gen_Ptr>(gettid());
    return lp_thread->disp(pp_arg);
}

void SB_Thread::Thread::start() {
    if (ip_id == NULL) { // allow multiple starts
#ifdef SB_THREAD_PRINT_THREAD_CALLS
        trace_printf("Thread::start() ENTRY, name=%s\n", ip_name);
#endif
        ip_id = Sthr::create(ip_name, thread_fun, this);
        if (iv_daemon & (ip_id != NULL)) {
            int lv_ret = Sthr::detach(ip_id);
            SB_util_assert_ieq(lv_ret, 0); // sw fault
            lv_ret = lv_ret; // touch (in case assert disabled)
        }
#ifdef SB_THREAD_PRINT_THREAD_CALLS
        trace_printf("Thread::start() EXIT, ret=%p\n", ip_id);
#endif
    }
    SB_util_assert_pne(ip_id, NULL); // sw fault
}

void SB_Thread::Thread::stop() {
    if (ip_id != NULL) { // ignore if nothing started
#ifdef SB_THREAD_PRINT_THREAD_CALLS
        trace_printf("Thread::stop() ENTRY, name=%s\n", ip_name);
#endif
        int lv_ret = Sthr::cancel(ip_id);
#ifdef SB_THREAD_PRINT_THREAD_CALLS
        trace_printf("Thread::stop() EXIT, ret=%d\n", lv_ret);
#endif
        SB_util_assert_ieq(lv_ret, 0); // sw fault
        lv_ret = lv_ret; // touch (in case assert disabled)
        ip_id = NULL;
    }
}

void SB_Thread::Thread::yield() {
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::yield() ENTRY, name=%s\n", ip_name);
#endif
    Sthr::yield();
#ifdef SB_THREAD_PRINT_THREAD_CALLS
    trace_printf("Thread::yield() EXIT, name=%s\n", ip_name);
#endif
}

// ----------------------------------------------------------------------------

SB_Thread::Sem::Sem() {
}

SB_Thread::Sem::~Sem() {
}
