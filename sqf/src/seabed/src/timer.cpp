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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/unistd.h>

#include <sys/time.h>

#include "seabed/fserr.h"
#include "seabed/pevents.h"
#include "seabed/timer.h"
#include "seabed/trace.h"

#include "apictr.h"
#include "compq.h"
#include "labelmapsx.h"
#include "mserrmsg.h"
#include "mseventmgr.h"
#include "msx.h"
#include "mstrace.h"
#include "qid.h"
#include "slotmgr.h"
#include "threadtlsx.h"
#include "timeri.h"
#include "timerx.h"
#include "util.h"

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

enum { TIMER_MAX_TLES = 100 };
enum { TIMER_SIG      = SIGALRM };

class SB_Timer_Comp_Queue : public SB_Ts_D_Queue {
public:
    SB_Timer_Comp_Queue(int pv_qid, const char *pp_name = NULL);
    ~SB_Timer_Comp_Queue() {}
    virtual void  add(SB_DQL_Type *pp_item);
    virtual void  lock();
    virtual void *remove();
    virtual bool  remove_list(SB_DQL_Type *pp_item);
    virtual void  unlock();

    int           get_tid();

private:
    void          remove_account(void *pp_item);

    int iv_tid;
};

typedef long long SB_To;
typedef struct Timer_TLE_Type {
    SB_DQL_Type            iv_link;   // MUST be first (for SB_D_Queue)
    bool                   iv_inuse;
    Timer_TLE_Type        *ip_prev;   // used for timer-list only
    Timer_TLE_Type        *ip_next;   // used for timer-list only
    Timer_List_Type        iv_on_list;
    short                  iv_tleid;
    Timer_TLE_Kind_Type    iv_kind;
    SB_Timer_Comp_Queue   *ip_comp_q;
    Timer_Cb_Type          iv_cb;
    SB_Ms_Event_Mgr       *ip_mgr;
    struct timeval         iv_comp_q_off_tod;
    struct timeval         iv_comp_q_on_tod;
    int                    iv_stid;   // source tid
    int                    iv_ttid;   // target tid
    SB_To                  iv_to;
    int                    iv_toval;
    short                  iv_parm1;
    long                   iv_parm2;
} Timer_TLE_Type;

typedef struct Timer_TQ_Node {
    SB_ML_Type           iv_link;
    int                  iv_tid;
    SB_Timer_Comp_Queue *ip_comp_q;
} Timer_TQ_Node;

//
// Handle timer signals
//
class SB_Timer_Thread : public SB_Thread::Thread {
public:
    SB_Timer_Thread(const char *pp_name);
    virtual ~SB_Timer_Thread();

    void run();
    bool running();
    void shutdown();
    void wait();

private:
    SB_Thread::CV iv_cv;
    bool          iv_running;
    bool          iv_shutdown;
};


// forwards
static void sb_timer_key_dtor(void *pp_queue);

// globals
static Timer_TLE_Type   *gp_timer_head = NULL;
static Timer_TLE_Type   *gp_timer_tail = NULL;
static SB_Slot_Mgr      *gp_timer_tle_mgr = NULL;
static Timer_TLE_Type   *gp_timer_tles = NULL;
static SB_Timer_Thread  *gp_timer_thr = NULL;
static bool              gv_timer_alloc = true;
static SB_Thread::ECM    gv_timer_mutex("mutex-gv_timer_mutex");
static struct itimerval  gv_timer_to;
static SB_Ts_Lmap        gv_timer_tpop_map("map-timer-tpop");
static SB_Ts_Lmap        gv_timer_tpop_tid_map("map-timer-tpop-tid");
static int               gv_timer_tls_inx =
                           SB_create_tls_key(sb_timer_key_dtor,
                                             "timer-compq");


// forwards
static void            sb_timer_comp_q_print(SB_Timer_Comp_Queue *pp_comp_q);
static bool            sb_timer_setitimer(const char *pp_where, SB_To pv_to);
static void            sb_timer_setitimer_retry(const char *pp_where,
                                                SB_To       pv_to);
static void            sb_timer_timer_list_complete(const char *pp_where);
static void            sb_timer_timer_list_print();
static Timer_TLE_Type *sb_timer_tle_alloc(const char          *pp_where,
                                          Timer_TLE_Kind_Type  pv_kind);
static void            sb_timer_tle_free(Timer_TLE_Type *pp_tle);
static void            sb_timer_tle_insert(const char     *pp_where,
                                           SB_To           pv_to,
                                           Timer_TLE_Type *pp_tle);
static SB_To           sb_timer_to_calc(int pv_toval);
static void            sb_timer_to_fmt(char *pp_fmt, SB_To pv_to);

#define RETURNFSCC(fserr) \
return (fserr >= XZFIL_ERR_BADERR) ? -1 : (fserr == XZFIL_ERR_OK) ? 0 : fserr;

SB_Timer_Comp_Queue::SB_Timer_Comp_Queue(int pv_qid, const char *pp_name)
: SB_Ts_D_Queue(pv_qid, pp_name) {
    iv_tid = gettid();
}

void SB_Timer_Comp_Queue::add(SB_DQL_Type *pp_item) {
    const char     *WHERE = "SB_Timer_Comp_Queue::add";
    char            la_to[40];
    Timer_TLE_Type *lp_tle;

    lp_tle = reinterpret_cast<Timer_TLE_Type *>(pp_item);
    if (gv_ms_trace_params) {
        gettimeofday(&lp_tle->iv_comp_q_on_tod, NULL);
        sb_timer_to_fmt(la_to, lp_tle->iv_to);
        trace_where_printf(WHERE, "adding to comp-q qid=%d(%s), tleid=%d, tle=%p, to=%s\n",
                           iv_qid, ia_d_q_name,
                           lp_tle->iv_tleid, pfp(lp_tle), la_to);
    }
    SB_Ts_D_Queue::add(pp_item);
}

int SB_Timer_Comp_Queue::get_tid() {
    return iv_tid;
}

void SB_Timer_Comp_Queue::lock() {
    // shared lock with recv-q for efficiency
    gv_ms_recv_q.lock();
}

void *SB_Timer_Comp_Queue::remove() {
    void *lp_ret;

    lp_ret = SB_Ts_D_Queue::remove();
    if (lp_ret != NULL) {
        if (gv_ms_trace_params)
            remove_account(lp_ret);
    }
    return lp_ret;
}

void SB_Timer_Comp_Queue::remove_account(void *pp_item) {
    const char     *WHERE = "SB_Timer_Comp_Queue::remove_account";
    Timer_TLE_Type *lp_tle;
    long long       lv_delta;

    lp_tle = static_cast<Timer_TLE_Type *>(pp_item);
    gettimeofday(&lp_tle->iv_comp_q_off_tod, NULL);
    lv_delta = (lp_tle->iv_comp_q_off_tod.tv_sec * SB_US_PER_SEC +
                lp_tle->iv_comp_q_off_tod.tv_usec) -
               (lp_tle->iv_comp_q_on_tod.tv_sec * SB_US_PER_SEC +
                lp_tle->iv_comp_q_on_tod.tv_usec);
    trace_where_printf(WHERE,
                       "comp-q-to-de-queue qid=%d(%s), (tleid=%d, tle=%p) time=%lld us\n",
                       iv_qid, ia_d_q_name,
                       lp_tle->iv_tleid, pfp(lp_tle),
                       lv_delta);
}

bool SB_Timer_Comp_Queue::remove_list(SB_DQL_Type *pp_item) {
    bool lv_ret;

    lv_ret = SB_Ts_D_Queue::remove_list(pp_item);
    if (lv_ret) {
        if (gv_ms_trace_params)
            remove_account(pp_item);
    }
    return lv_ret;
}

static void *sb_timer_thread_fun(void *pp_arg) {
    SB_Timer_Thread *lp_thread = static_cast<SB_Timer_Thread *>(pp_arg);
    lp_thread->run();
    return NULL;
}

SB_Timer_Thread::SB_Timer_Thread(const char *pp_name)
: Thread(sb_timer_thread_fun, pp_name),
  iv_running(false),
  iv_shutdown(false) {
}

void SB_Timer_Comp_Queue::unlock() {
    // shared lock with recv-q for efficiency
    gv_ms_recv_q.unlock();
}

SB_Timer_Thread::~SB_Timer_Thread() {
}

//
// wait for TIMER_SIG and process timer-list
//
void SB_Timer_Thread::run() {
    const char          *WHERE = "SB_Timer_Thread::run";
    int                  lv_err;
    int                  lv_sig;
    sigset_t             lv_set;
    int                  lv_status;

    if (gv_ms_trace_timer)
        trace_where_printf(WHERE, "timer sig thread started\n");

    iv_running = true;
    iv_cv.signal(true); // need lock

    sigemptyset(&lv_set);
    sigaddset(&lv_set, TIMER_SIG);
    while (!iv_shutdown) {
        lv_err = sigwait(&lv_set, &lv_sig);
        SB_util_assert_ieq(lv_err, 0);
        if (gv_ms_trace_timer)
            trace_where_printf(WHERE, "sigwait returned sig=%d\n", lv_sig);

        if (iv_shutdown)
            break;
        if (lv_sig != TIMER_SIG)
            continue;

        lv_status = gv_timer_mutex.lock();
        SB_util_assert_ieq(lv_status, 0);

        sb_timer_timer_list_complete(WHERE);

        if (gp_timer_head != NULL) {
            if (gv_ms_trace_timer)
                sb_timer_timer_list_print();
            for (;;) {
                if (gp_timer_head == NULL)
                    break;
                // restart timer
                if (sb_timer_setitimer(WHERE, gp_timer_head->iv_to))
                    break;
                sb_timer_timer_list_complete(WHERE);
            }
        }
        lv_status = gv_timer_mutex.unlock();
        SB_util_assert_ieq(lv_status, 0);
    }
    if (gv_ms_trace_timer)
        trace_where_printf(WHERE, "EXITING timer sig thread\n");
    iv_running = false;
}

bool SB_Timer_Thread::running() {
    return iv_running;
}

void SB_Timer_Thread::shutdown() {
    int lv_status;

    iv_shutdown = true;
    while (iv_running) {
        lv_status = kill(TIMER_SIG);
        if (lv_status == ESRCH)
            SB_util_assert(!iv_running);
        else
            SB_util_assert_ieq(lv_status, 0);
        SB_Thread::Sthr::usleep(100); // shutdown
    }
}

void SB_Timer_Thread::wait() {
    int lv_status;

    lv_status = iv_cv.wait(true); // need lock
    SB_util_assert_ieq(lv_status, 0);
}

void sb_timer_alloc() {
    int lv_inx;

    gp_timer_tles = new Timer_TLE_Type[TIMER_MAX_TLES];
    for (lv_inx = 0; lv_inx < TIMER_MAX_TLES; lv_inx++) {
        gp_timer_tles[lv_inx].iv_inuse = false;
        gp_timer_tles[lv_inx].iv_tleid = static_cast<short>(lv_inx);
    }

    gp_timer_tle_mgr = new SB_Slot_Mgr("slotmgr-timer-tle", SB_Slot_Mgr::ALLOC_FIFO, TIMER_MAX_TLES);
    lv_inx = gp_timer_tle_mgr->alloc();
    gp_timer_tles[lv_inx].iv_inuse = true;
    gv_timer_alloc = false;
}

static short sb_timer_cancel_com(const char *pp_where,
                                 short       pv_tag,
                                 bool        pv_cc) {
    SB_Timer_Comp_Queue *lp_comp_q;
    Timer_TLE_Type      *lp_tle;
    short                lv_fserr;
    bool                 lv_start_timer;
    int                  lv_status;
    SB_To                lv_to;

    if (gv_ms_trace_params)
        trace_where_printf(pp_where, "ENTER tag=%d\n", pv_tag);

    if (gv_timer_alloc)
        sb_timer_alloc();

    if ((pv_tag <= 0) || (pv_tag >= TIMER_MAX_TLES)) {
        if (gv_ms_trace_timer)
            trace_where_printf(pp_where, "tag out-of-bounds\n");
        if (pv_cc)
            lv_fserr = XZFIL_ERR_INVALOP; // CCG
        else
            lv_fserr = XZFIL_ERR_BOUNDSERR;
    } else {
        lv_status = gv_timer_mutex.lock();
        SB_util_assert_ieq(lv_status, 0);
        lp_tle = &gp_timer_tles[pv_tag];
        if (lp_tle->iv_inuse) {
            if (gp_timer_head == lp_tle) {
                // cancel head - may need to restart timer
                if (lp_tle->ip_next == NULL)
                    lv_to = 0;
                else
                    lv_to = lp_tle->ip_next->iv_to;
                lv_start_timer = true;
            } else {
                lv_to = 0; // initialize for compiler
                lv_start_timer = false;
            }

            lv_fserr = XZFIL_ERR_OK;
            // delink
            switch (lp_tle->iv_on_list) {
            case LIST_TIMER:
                if (gp_timer_head == lp_tle)
                    gp_timer_head = lp_tle->ip_next;
                if (gp_timer_tail == lp_tle)
                    gp_timer_tail = lp_tle->ip_prev;
                if (lp_tle->ip_prev != NULL)
                    lp_tle->ip_prev->ip_next = lp_tle->ip_next;
                if (lp_tle->ip_next != NULL)
                    lp_tle->ip_next->ip_prev = lp_tle->ip_prev;
                sb_timer_tle_free(lp_tle);
                break;

            case LIST_COMP:
                lp_comp_q = lp_tle->ip_comp_q;
                SB_util_assert_ieq(lp_tle->iv_kind, TIMER_TLE_KIND_COMPQ); // sw fault
                SB_util_assert_pne(lp_comp_q, NULL);                  // sw fault
                lp_comp_q->remove_list(&lp_tle->iv_link);
                if (gv_ms_trace_timer)
                    sb_timer_comp_q_print(lp_comp_q);
                sb_timer_tle_free(lp_tle);
                break;

            case LIST_NONE:
                // not on any list, can't cancel
                if (gv_ms_trace_timer)
                    trace_where_printf(pp_where, "TLE not on list\n");
                if (pv_cc)
                    lv_fserr = XZFIL_ERR_INVALOP; // CCG
                else
                    lv_fserr = XZFIL_ERR_INVALOP;
                break;

            default:
                SB_util_abort("invalid iv_on_list"); // sw fault
                break;
            }
            if (gv_ms_trace_timer)
                sb_timer_timer_list_print();

            if (lv_start_timer)
                sb_timer_setitimer_retry(pp_where, lv_to);
        } else {
            if (gv_ms_trace_timer)
                trace_where_printf(pp_where, "TLE not in use\n");
            if (pv_cc)
                lv_fserr = XZFIL_ERR_INVALOP; // CCG
            else
                lv_fserr = XZFIL_ERR_NOTFOUND;
        }
        lv_status = gv_timer_mutex.unlock();
        SB_util_assert_ieq(lv_status, 0);
    }

    if (gv_ms_trace_params)
        trace_where_printf(pp_where, "EXIT ret=%d\n", lv_fserr);
    return lv_fserr;
}

SB_Timer_Comp_Queue *sb_timer_comp_q_get() {
    SB_Timer_Comp_Queue *lp_comp_q;
    int                  lv_status;

    lp_comp_q =
      static_cast<SB_Timer_Comp_Queue *>(SB_Thread::Sthr::specific_get(gv_timer_tls_inx));
    if (lp_comp_q == NULL) {
        lp_comp_q = new SB_Timer_Comp_Queue(QID_TPOP_COMP, "q-tpop-comp-tle");
        Timer_TQ_Node *lp_node = new Timer_TQ_Node;
        lp_node->iv_link.iv_id.l = reinterpret_cast<long>(lp_comp_q);
        lp_node->iv_tid = gettid();
        gv_timer_tpop_map.put(&lp_node->iv_link);
        lv_status = SB_Thread::Sthr::specific_set(gv_timer_tls_inx, lp_comp_q);
        SB_util_assert_ieq(lv_status, 0);
        lp_node = new Timer_TQ_Node;
        lp_node->iv_link.iv_id.l = gettid();
        lp_node->ip_comp_q = lp_comp_q;
        gv_timer_tpop_tid_map.put(&lp_node->iv_link);
    }
    return lp_comp_q;
}

SB_Timer_Comp_Queue *sb_timer_comp_q_get_tid(int pv_tid) {
    Timer_TQ_Node *lp_node;

    lp_node =
      static_cast<Timer_TQ_Node *>(gv_timer_tpop_tid_map.get(pv_tid));
    SB_util_assert_pne(lp_node, NULL); // sw fault
    return lp_node->ip_comp_q;
}

bool sb_timer_comp_q_empty() {
    SB_Timer_Comp_Queue *lp_comp_q;

    lp_comp_q = sb_timer_comp_q_get();
    return lp_comp_q->empty();
}

//
// print completion list
//
void sb_timer_comp_q_print(SB_Timer_Comp_Queue *pp_comp_q) {
    char            la_to[40];
    Timer_TLE_Type *lp_tle;

    lp_tle = reinterpret_cast<Timer_TLE_Type *>(pp_comp_q->head());
    if (lp_tle != NULL)
        trace_printf("TIMER comp-q q=%p, tid=%d\n",
                     pfp(pp_comp_q), pp_comp_q->get_tid());
    while (lp_tle != NULL) {
        sb_timer_to_fmt(la_to, lp_tle->iv_to);
        trace_printf("tle id=%d, addr=%p, p=%p, n=%p, q=%p, stid=%d, ttid=%d, mgr=%p, to=%s, toval=%d, p1=%d(0x%x), p2=%ld(0x%lx)\n",
                     lp_tle->iv_tleid,
                     pfp(lp_tle),
                     pfp(lp_tle->iv_link.ip_prev),
                     pfp(lp_tle->iv_link.ip_next),
                     pfp(pp_comp_q),
                     lp_tle->iv_stid,
                     lp_tle->iv_ttid,
                     pfp(lp_tle->ip_mgr),
                     la_to,
                     lp_tle->iv_toval,
                     lp_tle->iv_parm1,
                     lp_tle->iv_parm1,
                     lp_tle->iv_parm2,
                     lp_tle->iv_parm2);
        lp_tle = reinterpret_cast<Timer_TLE_Type *>(lp_tle->iv_link.ip_next);
    }
}

void *sb_timer_comp_q_remove() {
    SB_Timer_Comp_Queue *lp_comp_q;
    Timer_TLE_Type      *lp_tle;
    int                  lv_status;

    lv_status = gv_timer_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    lp_comp_q = sb_timer_comp_q_get();
    lp_tle = static_cast<Timer_TLE_Type *>(lp_comp_q->remove());
    if (lp_tle != NULL)
        lp_tle->iv_on_list = LIST_NONE;
    lv_status = gv_timer_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);

    if (gv_ms_trace_timer)
        sb_timer_comp_q_print(lp_comp_q);
    return lp_tle;
}

//
// timer-module hi
//
int sb_timer_hi() {
    int lv_ret;

    if (gp_timer_tle_mgr == NULL)
        lv_ret = 0;
    else
        lv_ret = gp_timer_tle_mgr->hi();
    return lv_ret;
}

static void sb_timer_key_dtor(void *pp_queue) {
    SB_Timer_Comp_Queue *lp_comp_q = static_cast<SB_Timer_Comp_Queue *>(pp_queue);
    if (lp_comp_q != NULL) {
        Timer_TQ_Node *lp_node =
          static_cast<Timer_TQ_Node *>(gv_timer_tpop_map.remove(reinterpret_cast<long>(lp_comp_q)));
        if (lp_node != NULL) {
            Timer_TQ_Node *lp_node2 =
              static_cast<Timer_TQ_Node *>(gv_timer_tpop_tid_map.remove(lp_node->iv_tid));
            delete lp_node2;
            // only delete if it hasn't already been deleted
            delete lp_node;
            delete lp_comp_q;
        }
    }
}

//
// timer-module init
//
void sb_timer_init() {
    int      lv_err;
    sigset_t lv_set;

    // need to block signal so that other threads will get block too
    sigemptyset(&lv_set);
    sigaddset(&lv_set, TIMER_SIG);
    lv_err = pthread_sigmask(SIG_BLOCK, &lv_set, NULL);
    SB_util_assert_ieq(lv_err, 0);

    // it_interval isn't used
    gv_timer_to.it_interval.tv_sec = 0;
    gv_timer_to.it_interval.tv_usec = 0;
}

//
// set SRE fields
//
void sb_timer_set_sre_tpop(BMS_SRE_TPOP *pp_sre, void *pp_tle) {
    int             lv_status;
    Timer_TLE_Type *lp_tle;

    lp_tle = static_cast<Timer_TLE_Type *>(pp_tle);
    pp_sre->sre_tleId = lp_tle->iv_tleid;
    pp_sre->sre_tleTOVal = lp_tle->iv_toval;
    pp_sre->sre_tleType = 0;
    pp_sre->sre_tleParm1 = lp_tle->iv_parm1;
    pp_sre->sre_tleParm2 = lp_tle->iv_parm2;
    lv_status = gv_timer_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    sb_timer_tle_free(lp_tle);
    lv_status = gv_timer_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Call setitimer (returns true if set ok)
//
bool sb_timer_setitimer(const char *pp_where, SB_To pv_to) {
    int   lv_err;
    bool  lv_ret;
    int   lv_sec;
    SB_To lv_to;

    lv_ret = true;
    if (pv_to == 0) {
        gv_timer_to.it_value.tv_sec = 0;
        gv_timer_to.it_value.tv_usec = 0;
    } else {
        lv_to = sb_timer_to_calc(0);
        lv_to = pv_to - lv_to;
        if (lv_to > 0) {
            lv_sec = static_cast<int>(lv_to / SB_US_PER_SEC);
            gv_timer_to.it_value.tv_sec = lv_sec;
            gv_timer_to.it_value.tv_usec =
              static_cast<__suseconds_t>(lv_to - static_cast<SB_To>(lv_sec) * SB_US_PER_SEC);
        } else
            lv_ret = false;
    }
    if (gv_ms_trace_timer) {
        if (lv_ret)
            trace_where_printf(pp_where,
                               "setitimer to=%ld.%06ld\n",
                               gv_timer_to.it_value.tv_sec,
                               gv_timer_to.it_value.tv_usec);
        else
            trace_where_printf(pp_where, "setitimer not set\n");
    }
    if (lv_ret) {
        lv_err = setitimer(ITIMER_REAL, &gv_timer_to, NULL);
        SB_util_assert_ieq(lv_err, 0);
    }
    return lv_ret;
}

void sb_timer_setitimer_retry(const char *pp_where, SB_To pv_to) {
    while (!sb_timer_setitimer(pp_where, pv_to)) {
        // itimer not set - timer already expired
        if (gv_ms_trace_timer)
            trace_where_printf(pp_where, "setitimer not set - try to complete timers\n");
        sb_timer_timer_list_complete(pp_where);
        if (gp_timer_head == NULL)
            break;
    }
}

//
// timer-start (common)
//
static short sb_timer_start_com(const char          *pp_where,
                                bool                 pv_cc,
                                Timer_TLE_Kind_Type  pv_kind,
                                int                  pv_toval,
                                short                pv_parm1,
                                long                 pv_parm2,
                                short               *pp_tleid,
                                pid_t                pv_tid,
                                SB_Ms_Event_Mgr     *pp_mgr,
                                Timer_Cb_Type        pv_callback) {
    Timer_TLE_Type *lp_tle;
    short           lv_fserr;
    int             lv_status;
    short           lv_tleid;
    SB_To           lv_to;

    if (gv_timer_alloc)
        sb_timer_alloc();

    lv_status = gv_timer_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    lp_tle = sb_timer_tle_alloc(pp_where, pv_kind);
    if (lp_tle == NULL) {
        if (pv_cc)
            lv_fserr = XZFIL_ERR_BADERR; // CCL
        else
            lv_fserr = XZFIL_ERR_NOTFOUND;
        lv_tleid = -1;
    } else {
        lv_fserr = XZFIL_ERR_OK;
        switch (pv_kind) {
        case TIMER_TLE_KIND_CB:
            lp_tle->ip_mgr = NULL;
            lp_tle->iv_stid = gettid();
            lp_tle->iv_ttid = lp_tle->iv_stid;
            lp_tle->ip_comp_q = NULL;
            lp_tle->iv_cb = pv_callback;
            break;

        case TIMER_TLE_KIND_COMPQ:
            lp_tle->ip_mgr = pp_mgr;
            lp_tle->iv_stid = gettid();
            lp_tle->iv_cb = NULL;
            if (pv_tid == 0) {
                lp_tle->iv_ttid = lp_tle->iv_stid;
                lp_tle->ip_comp_q = sb_timer_comp_q_get();
            } else {
                lp_tle->iv_ttid = pv_tid;
                lp_tle->ip_comp_q = sb_timer_comp_q_get_tid(pv_tid);
            }
            break;

        default:
            break;
        }

        lv_to = sb_timer_to_calc(pv_toval);
        lp_tle->iv_to = lv_to;
        lp_tle->iv_toval = pv_toval;
        lp_tle->iv_parm1 = pv_parm1;
        lp_tle->iv_parm2 = pv_parm2;
        lv_tleid = lp_tle->iv_tleid;
        sb_timer_tle_insert(pp_where, lv_to, lp_tle);
        if (gv_ms_trace_timer)
            sb_timer_timer_list_print();
    }

    lv_status = gv_timer_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);

    *pp_tleid = lv_tleid;
    if (gv_ms_trace_params)
        trace_where_printf(pp_where, "EXIT tleid=%d, ret=%d\n",
                           lv_tleid, lv_fserr);
    return lv_fserr;
}

//
// allocate TLE
//
Timer_TLE_Type *sb_timer_tle_alloc(const char          *pp_where,
                                   Timer_TLE_Kind_Type  pv_kind) {
    Timer_TLE_Type *lp_tle;
    int             lv_tleid;

    if (gp_timer_tle_mgr->size() == TIMER_MAX_TLES)
        return NULL;
    lv_tleid = gp_timer_tle_mgr->alloc();
    lp_tle = &gp_timer_tles[lv_tleid];
    lp_tle->iv_inuse = true;
    lp_tle->iv_kind = pv_kind;
    lp_tle->iv_on_list = LIST_NONE;
    if (gv_ms_trace_timer)
        trace_where_printf(pp_where, "alloc tleid=%d\n", lv_tleid);
    return lp_tle;
}

//
// free TLE
//
void sb_timer_tle_free(Timer_TLE_Type *pp_tle) {
    const char *WHERE = "sb_timer_tle_free";
    int lv_tleid;

    lv_tleid = pp_tle->iv_tleid;
    pp_tle->iv_inuse = false;
    gp_timer_tle_mgr->free_slot(lv_tleid);
    if (gv_ms_trace_timer)
        trace_where_printf(WHERE, "free tleid=%d\n", lv_tleid);
}

//
// insert TLE into timer-ordered list
//
void sb_timer_tle_insert(const char     *pp_where,
                         SB_To           pv_to,
                         Timer_TLE_Type *pp_tle) {
    char            la_name[20];
    Timer_TLE_Type *lp_tle;

    lp_tle = gp_timer_head;
    // try to insert
    while (lp_tle != NULL) {
        if (pv_to < lp_tle->iv_to) {
            pp_tle->ip_prev = lp_tle->ip_prev;
            pp_tle->ip_next = lp_tle;
            if (lp_tle->ip_prev == NULL)
                gp_timer_head = pp_tle;
            else
                pp_tle->ip_prev->ip_next = pp_tle;
            lp_tle->ip_prev = pp_tle;
            pp_tle->iv_on_list = LIST_TIMER;
            break;
        }
        lp_tle = lp_tle->ip_next;
    }
    if (lp_tle == NULL) {
        // not inserted, insert at tail
        pp_tle->ip_prev = gp_timer_tail;
        pp_tle->ip_next = NULL;
        if (gp_timer_head == NULL)
            gp_timer_head = pp_tle;
        else
            gp_timer_tail->ip_next = pp_tle;
        gp_timer_tail = pp_tle;
        pp_tle->iv_on_list = LIST_TIMER;
    }

    if (gp_timer_head == pp_tle) {
        // set new timer if new head-of-list
        if (gp_timer_thr == NULL) {
            strcpy(la_name, "timer-sig");
            gp_timer_thr = new SB_Timer_Thread(la_name);
            if (gv_ms_trace_timer)
                trace_where_printf(pp_where, "starting timer sig thread %s\n",
                                   la_name);
            gp_timer_thr->start();
            gp_timer_thr->wait(); // wait for thread to get started
        }

        sb_timer_setitimer_retry(pp_where, pv_to);
    }
}

//
// timer-module shutdown
//
void sb_timer_shutdown() {
    const char *WHERE = "sb_timer_shutdown";
    enum      { MAX_TIMER_NODES = 10 };
    long        la_node[MAX_TIMER_NODES];
    void       *lp_result;
    int         lv_inx;
    int         lv_max;

    if (gv_ms_trace_params || gv_ms_trace_timer)
        trace_where_printf(WHERE, "ENTER\n");
    if (gp_timer_thr != NULL) {
        gp_timer_thr->shutdown();
        gp_timer_thr->join(&lp_result);
        SB_util_assert_peq(lp_result, NULL); // sw fault
        delete gp_timer_thr;
        gp_timer_thr = NULL;
    }
    delete [] gp_timer_tles;

    // remove
    while (gv_timer_tpop_map.size()) {
        SB_Lmap_Enum *lp_enum = gv_timer_tpop_map.keys();
        for (lv_max = 0;
             lp_enum->more() && (lv_max < MAX_TIMER_NODES);
             lv_max++)
            la_node[lv_max] = lp_enum->next()->iv_id.l;
        for (lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            Timer_TQ_Node *lp_node =
              static_cast<Timer_TQ_Node *>(gv_timer_tpop_map.remove(la_node[lv_inx]));
            delete lp_node;
        }
        delete lp_enum;
    }
    while (gv_timer_tpop_tid_map.size()) {
        SB_Lmap_Enum *lp_enum = gv_timer_tpop_tid_map.keys();
        for (lv_max = 0;
             lp_enum->more() && (lv_max < MAX_TIMER_NODES);
             lv_max++)
            la_node[lv_max] = lp_enum->next()->iv_id.l;
        for (lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            Timer_TQ_Node *lp_node =
              static_cast<Timer_TQ_Node *>(gv_timer_tpop_tid_map.remove(la_node[lv_inx]));
            delete lp_node;
        }
        delete lp_enum;
    }
    if (gv_ms_trace_params || gv_ms_trace_timer)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// timer-module size
//
int sb_timer_size() {
    int lv_ret;

    if (gp_timer_tle_mgr == NULL)
        lv_ret = 0;
    else
        lv_ret = gp_timer_tle_mgr->size();
    return lv_ret;
}

//
// complete timer list
//
void sb_timer_timer_list_complete(const char *pp_where) {
    char                 la_to[40];
    void                *lp_cb;
    SB_Timer_Comp_Queue *lp_comp_q;
    SB_Ms_Event_Mgr     *lp_mgr;
    Timer_TLE_Type      *lp_tle;
    SB_To                lv_over;
    int                  lv_status;
    short                lv_tle_tleid;
    int                  lv_tle_toval;
    short                lv_tle_parm1;
    long                 lv_tle_parm2;
    SB_To                lv_to;

    lv_to = sb_timer_to_calc(0);
    // process the head TLEs
    do {
        lp_tle = gp_timer_head;
        if (lp_tle == NULL)
            break;
        if (lp_tle->iv_to > lv_to)
            break;
        gp_timer_head = gp_timer_head->ip_next;
        if (gp_timer_head == NULL)
            gp_timer_tail = NULL;
        else
            gp_timer_head->ip_prev = NULL;

        lp_comp_q = lp_tle->ip_comp_q;
        if (gv_ms_trace_timer) {
            lv_over = lv_to - lp_tle->iv_to;
            sb_timer_to_fmt(la_to, lp_tle->iv_to);
            switch (lp_tle->iv_kind) {
            case TIMER_TLE_KIND_CB:
                lp_cb = SB_CB_TO_PTR(lp_tle->iv_cb);
                trace_where_printf(pp_where, "callback TLE to cb=%p, stid=%d, tleid=%d, to=%s, over=%lld\n",
                                   lp_cb,
                                   lp_tle->iv_stid,
                                   lp_tle->iv_tleid,
                                   la_to,
                                   lv_over);
                break;

            case TIMER_TLE_KIND_COMPQ:
                trace_where_printf(pp_where, "adding TLE to comp-q q=%p, stid=%d, ttid=%d, mgr=%p, tleid=%d, to=%s, over=%lld\n",
                                   pfp(lp_comp_q),
                                   lp_tle->iv_stid,
                                   lp_tle->iv_ttid,
                                   pfp(lp_tle->ip_mgr),
                                   lp_tle->iv_tleid,
                                   la_to,
                                   lv_over);
                break;

            default:
                SB_util_abort("invalid iv_kind"); // sw fault
            }
        }
        lp_mgr = lp_tle->ip_mgr;
        lp_tle->ip_next = NULL;
        lp_tle->ip_prev = NULL;
        switch (lp_tle->iv_kind) {
        case TIMER_TLE_KIND_CB:
            lp_tle->iv_on_list = LIST_NONE;
            lv_tle_tleid = lp_tle->iv_tleid;
            lv_tle_toval = lp_tle->iv_toval;
            lv_tle_parm1 = lp_tle->iv_parm1;
            lv_tle_parm2 = lp_tle->iv_parm2;
            sb_timer_tle_free(lp_tle);
            // unlock mutex so that cb can start another timer
            lv_status = gv_timer_mutex.unlock();
            SB_util_assert_ieq(lv_status, 0); // sw fault
            lp_tle->iv_cb(lv_tle_tleid,
                          lv_tle_toval,
                          lv_tle_parm1,
                          lv_tle_parm2);
            lv_status = gv_timer_mutex.lock();
            SB_util_assert_ieq(lv_status, 0); // sw fault
            break;
        case TIMER_TLE_KIND_COMPQ:
            lp_comp_q->add(&lp_tle->iv_link);
            if (gv_ms_trace_timer)
                sb_timer_comp_q_print(lp_comp_q);
            lp_tle->iv_on_list = LIST_COMP;
            lp_mgr->set_event(LREQ, NULL);
            break;
        default:
            SB_util_abort("invalid iv_kind"); // sw fault
        }
    } while (gp_timer_head != NULL);
}

//
// print timer list
//
void sb_timer_timer_list_print() {
    char            la_info[50];
    char            la_to[40];
    Timer_TLE_Type *lp_tle;
    Timer_TLE_Type *lp_tle_prev;

    lp_tle = gp_timer_head;
    lp_tle_prev = NULL;
    if (lp_tle != NULL)
        trace_printf("TIMER timer list\n");
    while (lp_tle != NULL) {
        sb_timer_to_fmt(la_to, lp_tle->iv_to);
        switch (lp_tle->iv_kind) {
        case TIMER_TLE_KIND_CB:
            sprintf(la_info, "cb=%p", SB_CB_TO_PTR(lp_tle->iv_cb));
            break;
        case TIMER_TLE_KIND_COMPQ:
            sprintf(la_info, "q=%p", pfp(lp_tle->ip_comp_q));
            break;
        default:
            strcpy(la_info, "?");
            SB_util_abort("invalid iv_kind"); // sw fault
            break;
        }
        trace_printf("tle id=%d, addr=%p, p=%p, n=%p, %s, stid=%d, ttid=%d, mgr=%p, to=%s, toval=%d, p1=%d(0x%x), p2=%ld(0x%lx)\n",
                     lp_tle->iv_tleid,
                     pfp(lp_tle),
                     pfp(lp_tle->ip_prev),
                     pfp(lp_tle->ip_next),
                     la_info,
                     lp_tle->iv_stid,
                     lp_tle->iv_ttid,
                     pfp(lp_tle->ip_mgr),
                     la_to,
                     lp_tle->iv_toval,
                     lp_tle->iv_parm1,
                     lp_tle->iv_parm1,
                     lp_tle->iv_parm2,
                     lp_tle->iv_parm2);
        SB_util_assert_peq(lp_tle->ip_prev, lp_tle_prev);
        lp_tle_prev = lp_tle;
        lp_tle = lp_tle->ip_next;
    }
}

//
// timeout calculation
//
SB_To sb_timer_to_calc(int pv_toval) {
    SB_To           lv_ret;
    struct timespec lv_ts;

    clock_gettime(CLOCK_REALTIME, &lv_ts);
    lv_ret = (static_cast<SB_To>(pv_toval) * SB_US_PER_TIC) +
             (static_cast<SB_To>(lv_ts.tv_sec) * SB_US_PER_SEC) +
             (static_cast<SB_To>(lv_ts.tv_nsec) / SB_NS_PER_US);
    return lv_ret;
}

//
// timeout format
//
void sb_timer_to_fmt(char *pp_fmt, SB_To pv_to) {
    struct tm      *lp_tx;
    int             lv_ms;
    time_t          lv_sec;
    struct tm       lv_tx;
    int             lv_us;

    lv_sec = static_cast<time_t>(pv_to / SB_US_PER_SEC);
    lv_us = static_cast<int>(pv_to - lv_sec * SB_US_PER_SEC);
    lp_tx = localtime_r(&lv_sec, &lv_tx);
    lv_ms = static_cast<int>(lv_us) / SB_US_PER_MS;
    lv_us = static_cast<int>(lv_us) - lv_ms * SB_US_PER_MS;
    sprintf(pp_fmt, "%02d:%02d:%02d.%03d.%03d",
            lp_tx->tm_hour, lp_tx->tm_min, lp_tx->tm_sec, lv_ms, lv_us);
}

//
// Purpose: A near close to XCANCELTIMEOUT
//
SB_Export int timer_cancel(short pv_tag) {
    const char *WHERE = "timer_cancel";
    short       lv_fserr;
    SB_API_CTR (lv_zctr, TIMER_CANCEL);

    lv_fserr = sb_timer_cancel_com(WHERE, pv_tag, false);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: print kind
//
const char *timer_print_kind(int pv_kind) {
    const char *lp_label = SB_get_label(&gv_timer_kind_label_map, pv_kind);
    printf("%s", lp_label);
    return lp_label;
}

//
// Purpose: print list
//
const char *timer_print_list(int pv_list) {
    const char *lp_label = SB_get_label(&gv_timer_list_label_map, pv_list);
    printf("%s", lp_label);
    return lp_label;
}

//
// Purpose: register tid
//
SB_Export int timer_register() {
    const char *WHERE = "timer_register";
    short       lv_fserr;
    SB_API_CTR (lv_zctr, TIMER_REGISTER);

    lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    sb_timer_comp_q_get();
    gv_ms_event_mgr.get_mgr(NULL);
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: A near close to XSIGNALTIMEOUT, but callback is called on to
//
SB_Export int timer_start_cb(int            pv_toval,
                             short          pv_parm1,
                             long           pv_parm2,
                             short         *pp_tleid,
                             Timer_Cb_Type  pv_callback) {
    const char *WHERE = "timer_start_cb";
    void       *lp_cb;
    short       lv_fserr;
    SB_API_CTR (lv_zctr, TIMER_START_CB);

    if (gv_ms_trace_params) {
        lp_cb = SB_CB_TO_PTR(pv_callback);
        trace_where_printf(WHERE,
                           "ENTER toval=%d, parm1=%d(0x%x), parm2=%ld(0x%lx), tleid=%p, cb=%p\n",
                           pv_toval, pv_parm1, pv_parm1, pv_parm2, pv_parm2,
                           pfp(pp_tleid), lp_cb);
    }

    lv_fserr = sb_timer_start_com(WHERE,
                                  false,
                                  TIMER_TLE_KIND_CB,
                                  pv_toval,
                                  pv_parm1,
                                  pv_parm2,
                                  pp_tleid,
                                  0,
                                  NULL,
                                  pv_callback);

    return ms_err_rtn(lv_fserr);
}

//
// Purpose: emulate CANCELTIMEOUT
//
SB_Export _xcc_status XCANCELTIMEOUT(short pv_tag) {
    const char *WHERE = "XCANCELTIMEOUT";
    int         lv_fserr;
    SB_API_CTR (lv_zctr, XCANCELTIMEOUT);

    lv_fserr = sb_timer_cancel_com(WHERE, pv_tag, true);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate SIGNALTIMEOUT
//
SB_Export _xcc_status XSIGNALTIMEOUT(int    pv_toval,
                                     short  pv_parm1,
                                     long   pv_parm2,
                                     short *pp_tleid,
                                     pid_t  pv_tid) {
    const char      *WHERE = "XSIGNALTIMEOUT";
    SB_Ms_Event_Mgr *lp_mgr;
    int              lv_fserr;
    short            lv_tleid;
    SB_API_CTR      (lv_zctr, XSIGNALTIMEOUT);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER toval=%d, parm1=%d(0x%x), parm2=%ld(0x%lx), tleid=%p, ttid=%d\n",
                           pv_toval, pv_parm1, pv_parm1, pv_parm2, pv_parm2, pfp(pp_tleid), pv_tid);
    if (pv_tid == 0)
        lp_mgr = gv_ms_event_mgr.get_mgr(NULL);
    else {
        lp_mgr = gv_ms_event_mgr.get_mgr_tid(pv_tid);
        if (lp_mgr == NULL) {
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "ttid=%d not registered\n", pv_tid);
            lv_fserr = XZFIL_ERR_BADERR; // CCL
            lv_tleid = -1;
            *pp_tleid = lv_tleid;
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "EXIT tleid=%d, ret=%d\n",
                                   lv_tleid, lv_fserr);

            RETURNFSCC(lv_fserr);
        }
    }

    lv_fserr = sb_timer_start_com(WHERE,
                                  true,
                                  TIMER_TLE_KIND_COMPQ,
                                  pv_toval,
                                  pv_parm1,
                                  pv_parm2,
                                  pp_tleid,
                                  pv_tid,
                                  lp_mgr,
                                  NULL);

    RETURNFSCC(lv_fserr);
}

