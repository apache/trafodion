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
// Implement SB_Thread class
//
// These defines may be enabled for this implementation:
//   SB_THREAD_NONPREEMPTIVE - manipulates Preemptive_Mutex::[try|un]lock
//   SB_THREAD_PRINT_PTHREAD_CALLS - prints outcome of pthread calls
//

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/unistd.h> // gettid
#include <sys/time.h>

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

#include "common/evl_sqlog_eventnum.h"

#include "seabed/log.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "buf.h"
#include "cap.h"
#include "chk.h"
#include "logaggr.h"
#include "mstrace.h"
#include "tablemgr.h"
#include "threadtlsx.h"
#include "util.h"
#include "utracex.h"


#ifndef USE_SB_INLINE
#include "seabed/int/thread.inl"
#endif


typedef struct SB_Thread_Ctx_Type {
    char      ia_thread_name[20];
    void     *ip_fun;
    int       iv_ctx_id;
    pthread_t iv_self;
    pid_t     iv_tid;
} SB_Thread_Ctx_Type;

class SB_Thread_Table_Entry_Mgr : public SB_Table_Entry_Mgr<SB_Thread_Ctx_Type> {
public:
    SB_Thread_Table_Entry_Mgr() {}

    virtual ~SB_Thread_Table_Entry_Mgr() {}

    SB_Thread_Ctx_Type *entry_alloc(size_t pv_inx) {
        SB_Thread_Ctx_Type *lp_ctx;
        pv_inx = pv_inx; // touch
        lp_ctx = new SB_Thread_Ctx_Type;
        return lp_ctx;
    }

    void entry_alloc_block(SB_Thread_Ctx_Type **ppp_table,
                           size_t               pv_inx,
                           size_t               pv_count) {
        ppp_table = ppp_table; // touch
        pv_inx = pv_inx; // touch
        pv_count = pv_count; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_cap_change(size_t pv_inx, size_t pv_cap) {
        const char *WHERE = "SB_Thread_Table_Entry_Mgr::entry_cap_change";

        gv_ms_cap.iv_thread_table = pv_cap;
        if (pv_inx && ((pv_cap % LOG_HIGH) == 0)) {
            sb_log_aggr_cap_log(WHERE, SB_LOG_AGGR_CAP_THREAD_TABLE_CAP_INC,
                                static_cast<int>(pv_cap));
            if (gv_ms_trace)
                trace_where_printf(WHERE,
                                   "THREAD-table capacity increased new-cap=%d\n",
                                   static_cast<int>(pv_cap));
        }
    }

    bool entry_free(SB_Thread_Ctx_Type *pp_ctx, size_t pv_inx, bool pv_urgent) {
        pv_inx = pv_inx; // touch
        pv_urgent = pv_urgent; // touch
        delete pp_ctx;
        return true;
    }

    void entry_free_block(SB_Thread_Ctx_Type **ppp_table,
                          size_t               pv_inx,
                          size_t               pv_count) {
        ppp_table = ppp_table; // touch
        pv_inx = pv_inx; // touch
        pv_count = pv_count; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_inuse(SB_Thread_Ctx_Type *pp_ctx,
                     size_t              pv_inx,
                     bool                pv_inuse) {
        pp_ctx = pp_ctx; // touch
        pv_inx = pv_inx; // touch
        pv_inuse = pv_inuse; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_print_str(SB_Thread_Ctx_Type *pp_ctx,
                         size_t              pv_inx,
                         char               *pp_info) {
        pp_ctx = pp_ctx; // touch
        pv_inx = pv_inx; // touch
        pp_info = pp_info; // touch
    }

private:
    enum { LOG_HIGH = 10 }; // log every time high moves by this much
};

// forwards
static void sb_thread_ctx_key_dtor(void *pp_ctx);
static void sb_thread_name_key_dtor(void *pp_name);

SB_Ts_Table_Mgr<SB_Thread_Ctx_Type> *gv_sb_thread_table = NULL;
SB_Thread_Table_Entry_Mgr           *gv_sb_thread_table_entry_mgr = NULL;
static int                          gv_sb_thread_ctx_tls_inx =
                                      SB_create_tls_key(sb_thread_ctx_key_dtor,
                                                        "thread-ctx");
static int                          gv_sb_thread_name_tls_inx =
                                      SB_create_tls_key(sb_thread_name_key_dtor,
                                                        "thread-name");

#ifdef SB_THREAD_LOCK_STATS
SB_Thread_Lock_Stats                gv_sb_lock_stats;
#endif

SB_Ts_Table_Mgr<SB_Thread_Ctx_Type> *getGlobalTsTableMgr() {
  if (gv_sb_thread_table != NULL)
     return gv_sb_thread_table;
  SB_util_short_lock();
  if (gv_sb_thread_table != NULL) {
     SB_util_short_unlock();
     return gv_sb_thread_table;
  }
  gv_sb_thread_table_entry_mgr = new SB_Thread_Table_Entry_Mgr();
  gv_sb_thread_table = new SB_Ts_Table_Mgr<SB_Thread_Ctx_Type> ("tablemgr-THREAD-MGR",
                                           SB_Table_Mgr_Alloc::ALLOC_FAST,
                                           SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                           gv_sb_thread_table_entry_mgr,
                                           10, 10); // cap-init, cap-inc
  SB_util_short_unlock();
  return gv_sb_thread_table;
}


typedef struct Sthr_Disp {
    int                        iv_ctx_id;
    SB_Thread::Sthr::Function  iv_fun;
    void                      *ip_arg;
    char                      *ip_name;
} Sthr_Disp;

static void sb_thread_ctx_key_dtor(void *pp_ctx) {
    const char         *WHERE = "Sthr::create(via key-dtor)";
    SB_Thread_Ctx_Type *lp_ctx;

    lp_ctx = static_cast<SB_Thread_Ctx_Type *>(pp_ctx);
    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_THREAD_EXIT, lp_ctx->iv_tid);
    SB_UTRACE_API_ADDC(SB_UTRACE_API_OP_THREAD_EXIT_NAME,
                       lp_ctx->ia_thread_name);
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_THREAD_EXIT_DISP2,
                       gettid(),
                       reinterpret_cast<SB_Utrace_API_Info_Type>(lp_ctx->ip_fun));
    if (gv_sb_trace_thread)
        trace_where_printf(WHERE,
                           "thread exiting ctx-id=%d, name=%s\n",
                           lp_ctx->iv_ctx_id, lp_ctx->ia_thread_name);
    getGlobalTsTableMgr()->free_entry(lp_ctx->iv_ctx_id);
}

static void sb_thread_name_key_dtor(void *pp_name) {
    char *lp_name;

    if (pp_name != NULL) {
        lp_name = static_cast<char *>(pp_name);
        delete [] lp_name;
    }
}

int SB_Thread::Sthr::cancel(Sthr::Id_Ptr pp_tid) {
    pthread_t *lp_pthread_t;

    lp_pthread_t = static_cast<pthread_t *>(pp_tid);
    int lv_status = pthread_cancel(*lp_pthread_t);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_cancel(%p) EXIT, ret=%d\n", pp_tid, lv_status);
#endif
    return lv_status;
}

static void *sb_thread_sthr_disp(void *pp_arg) {
    const char                *WHERE = "Sthr::create(via)";
    char                       la_name[100];
    void                      *lp_arg;
    SB_Thread_Ctx_Type        *lp_ctx;
    Sthr_Disp                 *lp_disp;
    void                      *lp_fun;
    char                      *lp_name;
    void                      *lp_ret;
    int                        lv_ctx_id;
    SB_Thread::Sthr::Function  lv_fun;
    int                        lv_status;

    lp_disp = static_cast<Sthr_Disp *>(pp_arg);
    lv_fun = lp_disp->iv_fun;
    lp_fun = SB_CB_TO_PTR(lv_fun);
    lp_arg = lp_disp->ip_arg;
    lp_name = lp_disp->ip_name;
    lv_ctx_id = lp_disp->iv_ctx_id;
    lp_ctx = getGlobalTsTableMgr()->get_entry(lv_ctx_id);
    lp_ctx->ip_fun = lp_fun;
    lp_ctx->iv_self = pthread_self();
    lp_ctx->iv_tid = gettid();

    // in case thread calls exit - will get exit info this way
    lv_status = SB_Thread::Sthr::specific_set(gv_sb_thread_ctx_tls_inx, lp_ctx);
    SB_util_assert_ieq(lv_status, 0);

    if (gv_sb_trace_thread) {
        la_name[sizeof(la_name) - 1] = '\0';
        strncpy(la_name, lp_name, sizeof(la_name) - 1);
        trace_where_printf(WHERE, "thread started ctx-id=%d, name=%s, fun=%p, arg=%p\n",
                           lv_ctx_id, lp_name, lp_fun, lp_arg);
    }
    delete lp_disp;
    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_THREAD_START, lp_ctx->iv_tid);
    SB_UTRACE_API_ADDC(SB_UTRACE_API_OP_THREAD_START_NAME, lp_name);
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_THREAD_START_DISP2,
                       gettid(),
                       reinterpret_cast<SB_Utrace_API_Info_Type>(lp_fun));
    lp_ret = lv_fun(lp_arg);
    return lp_ret;
}

void SB_thread_main() {
    SB_Thread_Ctx_Type *lp_ctx;
    int                 lv_ctx_id;

    lv_ctx_id = static_cast<int>(getGlobalTsTableMgr()->alloc_entry());
    lp_ctx = getGlobalTsTableMgr()->get_entry(lv_ctx_id);
    lp_ctx->iv_ctx_id = lv_ctx_id;
    strcpy(lp_ctx->ia_thread_name, "main");
    lp_ctx->iv_self = pthread_self();
    lp_ctx->iv_tid = gettid();
}

SB_Thread::Sthr::Id_Ptr SB_Thread::Sthr::create(char           *pp_name,
                                                Sthr::Function  pv_fun,
                                                void           *pp_arg) {
    pthread_attr_t lv_attr;
    pthread_attr_init(&lv_attr);

    // create the thread id and thread
    pthread_t *lp_new_tid = new pthread_t;
    Sthr_Disp *lp_disp = new Sthr_Disp;
    lp_disp->iv_fun = pv_fun;
    lp_disp->ip_arg = pp_arg;
    lp_disp->ip_name = pp_name;

    lp_disp->iv_ctx_id = static_cast<int>(getGlobalTsTableMgr()->alloc_entry());
    SB_Thread_Ctx_Type *lp_ctx =
      getGlobalTsTableMgr()->get_entry(lp_disp->iv_ctx_id);
    lp_ctx->iv_ctx_id = lp_disp->iv_ctx_id;
    lp_ctx->ia_thread_name[sizeof(lp_ctx->ia_thread_name)-1] = '\0';
    strncpy(lp_ctx->ia_thread_name,
            pp_name,
            sizeof(lp_ctx->ia_thread_name) - 1);
    lp_ctx->iv_tid = 0;

    int lv_status = pthread_create(lp_new_tid,
                                   &lv_attr,
                                   sb_thread_sthr_disp,
                                   lp_disp);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_create(%p,%p,%p) EXIT, ret=%d\n",
                     static_cast<void *>(lp_new_tid),
                     static_cast<void *>(&lv_attr),
                     pp_arg,
                     lv_status);
#endif

    // if we failed, delete the tid we just created
    if (lv_status) {
        SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_THREAD_START_ERR, lv_status);
        delete lp_new_tid;
        lp_new_tid = NULL;
    }

    return static_cast<Sthr::Id_Ptr>(lp_new_tid);
}

void SB_Thread::Sthr::delete_id(Id_Ptr pp_tid) {
    pthread_t *lp_pthread_t;

    lp_pthread_t = static_cast<pthread_t *>(pp_tid);
    delete lp_pthread_t;
}

int SB_Thread::Sthr::detach(Sthr::Id_Ptr pp_tid) {
    pthread_t *lp_pthread_t;

    lp_pthread_t = static_cast<pthread_t *>(pp_tid);
    int lv_status = pthread_detach(*lp_pthread_t);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_detach(%p) EXIT, ret=%d\n",
                     static_cast<void *>(pp_tid), lv_status);
#endif
    return lv_status;
}

void SB_Thread::Sthr::exit(void *pp_value) {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_exit(%p) ENTRY\n", pp_value);
#endif
    pthread_exit(pp_value);
}

int SB_Thread::Sthr::join(Sthr::Id_Ptr pp_tid, void **ppp_result) {
    pthread_t *lp_pthread_t;
    int        lv_status;

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_join(%p) ENTRY\n", pp_tid);
#endif
    lp_pthread_t = static_cast<pthread_t *>(pp_tid);
    lv_status = pthread_join(*lp_pthread_t, ppp_result);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_join(%p,%p) EXIT, ret=%d\n",
                     pp_tid, *ppp_result, lv_status);
#endif
    return lv_status;
}

int SB_Thread::Sthr::kill(Sthr::Id_Ptr pp_tid, int pv_sig) {
    pthread_t *lp_pthread_t;
    int        lv_status;

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_kill(%p) ENTRY\n", pp_tid);
#endif
    lp_pthread_t = static_cast<pthread_t *>(pp_tid);
    lv_status = pthread_kill(*lp_pthread_t, pv_sig);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_kill(%p) EXIT, ret=%d\n",
                     pp_tid, lv_status);
#endif
    return lv_status;
}

char *SB_Thread::Sthr::self_name() {
    char *lp_name;

    lp_name = static_cast<char *>(SB_Thread::Sthr::specific_get(gv_sb_thread_name_tls_inx));
    return lp_name;
}

void SB_Thread::Sthr::set_name(char *pp_name) {
    char *lp_name;
    int   lv_status;

    lp_name =
      static_cast<char *>(SB_Thread::Sthr::specific_get(gv_sb_thread_name_tls_inx));
    if (lp_name == NULL) {
        lp_name = new char[strlen(pp_name) + 1];
        strcpy(lp_name, pp_name);
        lv_status = SB_Thread::Sthr::specific_set(gv_sb_thread_name_tls_inx, lp_name);
        SB_util_assert_ieq(lv_status, 0);
    }
}

// ----------------------------------------------------------------------------

SB_Thread::CV::CV() : iv_destroyed(false), iv_flag(false) {
    init(NULL);
}

SB_Thread::CV::CV(const char *pp_name) : iv_destroyed(false), iv_flag(false) {
    init(pp_name);
}

#ifdef SB_THREAD_LINUX
SB_Thread::CV::~CV() {
    int lv_status;

    lv_status = destroy();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    if (lv_status == 0) {
        lv_status = Mutex::destroy();
        SB_util_assert_ieq(lv_status, 0); // sw fault
    }
}
#else
SB_Thread::CV::~CV() {
    pthread_cond_t *lp_cv;
    int             lv_status;

    if (ip_cv != NULL) {
        lp_cv = reinterpret_cast<pthread_cond_t *>(ip_cv);
        lv_status = pthread_cond_destroy(lp_cv);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
        if (gv_sb_trace_pthread)
            trace_printf("pthread_cond_destroy(%p) EXIT, ret=%d\n",
                         static_cast<void *>(lp_cv), lv_status);
#endif
        SB_util_assert_ieq(lv_status, 0); // sw fault
        lv_status = lv_status; // touch (in case assert disabled)
        delete lp_cv;
    }
}
#endif

void SB_Thread::CV::init(const char *pp_name) {
    setname(pp_name);
#ifdef SB_THREAD_LINUX
    pthread_cond_t *lp_cv = &iv_cv;
#else
    pthread_cond_t *lp_cv = new pthread_cond_t;
    ip_cv = reinterpret_cast<Thr_Gen_Ptr>(lp_cv);
#endif
    int lv_status = pthread_cond_init(lp_cv, NULL);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_cond_init(%p) EXIT, ret=%d\n",
                     static_cast<void *>(lp_cv), lv_status);
#endif
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
}

// ----------------------------------------------------------------------------

SB_Thread::Mutex::Mutex()
: iv_destroyed(false), iv_errorcheck(false), iv_recursive(false) {
    init(NULL);
}

SB_Thread::Mutex::Mutex(const char *pp_name)
: iv_destroyed(false), iv_errorcheck(false), iv_recursive(false) {
    init(pp_name);
}

SB_Thread::Mutex::Mutex(bool pv_recursive)
: iv_destroyed(false), iv_errorcheck(false), iv_recursive(pv_recursive) {
    init(NULL);
}

SB_Thread::Mutex::Mutex(bool pv_recursive, bool pv_errorcheck)
: iv_destroyed(false), iv_errorcheck(pv_errorcheck), iv_recursive(pv_recursive) {
    SB_util_assert(!(pv_recursive && pv_errorcheck));
    init(NULL);
}

#ifdef SB_THREAD_LINUX
SB_Thread::Mutex::~Mutex() {
    int lv_status;

    lv_status = destroy();
    // NOTE: Disabling this assert:   SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
}
#else
SB_Thread::Mutex::~Mutex() {
    pthread_mutex_t *lp_mutex;
    int              lv_status;

    if (ip_mutex != NULL) {
        lp_mutex = reinterpret_cast<pthread_mutex_t *>(ip_mutex);
        lv_status = pthread_mutex_destroy(lp_mutex);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
        if (gv_sb_trace_pthread)
            trace_printf("pthread_mutex_destroy(%p) EXIT, ret=%d\n",
                         static_cast<void *>(lp_mutex), lv_status);
#endif
        SB_util_assert_ieq(lv_status, 0); // sw fault
        lv_status = lv_status; // touch (in case assert disabled)
        delete lp_mutex;
    }
}
#endif

void SB_Thread::Mutex::init(const char *pp_name) {
    setname(pp_name);
#ifdef SB_THREAD_LINUX
    pthread_mutex_t *lp_mutex = &iv_mutex;
#else
    pthread_mutex_t *lp_mutex = new pthread_mutex_t;
    ip_mutex = reinterpret_cast<Thr_Gen_Ptr>(lp_mutex);
#endif
    pthread_mutexattr_t lv_attr;
    int lv_status = pthread_mutexattr_init(&lv_attr);
    SB_util_assert_ieq(lv_status, 0); // sw fault
    if (iv_errorcheck) {
        lv_status = pthread_mutexattr_settype(&lv_attr,
                                              PTHREAD_MUTEX_ERRORCHECK);
        SB_util_assert_ieq(lv_status, 0); // sw fault
    }
    if (iv_recursive) {
        lv_status = pthread_mutexattr_settype(&lv_attr,
                                              PTHREAD_MUTEX_RECURSIVE);
        SB_util_assert_ieq(lv_status, 0); // sw fault
    }
    lv_status = pthread_mutex_init(lp_mutex, &lv_attr);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_mutex_init(%p) EXIT, ret=%d\n",
                     static_cast<void *>(lp_mutex), lv_status);
#endif
    SB_util_assert_ieq(lv_status, 0); // sw fault
#ifdef SB_THREAD_LOCK_STATS
    iv_last_locker = 0;
#endif
}

// ----------------------------------------------------------------------------

SB_Thread::Errorcheck_Mutex::Errorcheck_Mutex()
: Mutex(false, true), iv_locked(false) {
}

SB_Thread::Errorcheck_Mutex::Errorcheck_Mutex(bool pv_destructor_unlock)
: Mutex(false, true),
  iv_destructor_unlock(pv_destructor_unlock), iv_locked(false) {
}

SB_Thread::Errorcheck_Mutex::~Errorcheck_Mutex() {
    if (iv_destructor_unlock && iv_locked) {
        int lv_status = unlock();
        SB_util_assert_ieq(lv_status, 0); // sw fault
        lv_status = lv_status; // touch (in case assert disabled)
    }
}

// ----------------------------------------------------------------------------

#ifdef SB_THREAD_LINUX
SB_Thread::RWL::RWL() {
    int lv_status;

    ip_rwl_name = NULL;
    lv_status = pthread_rwlock_init(&iv_rwl, NULL);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_init(%p) EXIT, ret=%d\n",
                     static_cast<void *>(&iv_rwl), lv_status);
#endif
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
#ifdef SB_THREAD_LOCK_STATS
    iv_last_read_locker = 0;
    iv_last_write_locker = 0;
#endif
}
#else
#error "RWL::RWL() not implemented for platform"
#endif

#ifdef SB_THREAD_LINUX
SB_Thread::RWL::~RWL() {
    int lv_status;

    lv_status = pthread_rwlock_destroy(&iv_rwl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_destroy(%p) EXIT, ret=%d\n",
                     static_cast<void *>(&iv_rwl), lv_status);
#endif
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
}
#else
#error "RWL::~RWL() not implemented for platform"
#endif

// ----------------------------------------------------------------------------

SB_Thread::SL::SL(bool pv_pshared) {
    init(NULL, pv_pshared);
}

SB_Thread::SL::SL(const char *pp_name, bool pv_pshared) {
    init(pp_name, pv_pshared);
}

void SB_Thread::SL::init(const char *pp_name, bool pv_pshared) {
    ip_sl_name = pp_name;

#ifdef SB_THREAD_LINUX
    pthread_spinlock_t *lp_sl = &iv_sl;
#else
    pthread_spinlock_t *lp_sl = new pthread_spinlock_t;
    ip_sl = const_cast<Thr_Gen_Ptr>(lp_sl);
#endif
    int lv_pshared;
    if (pv_pshared)
        lv_pshared = PTHREAD_PROCESS_SHARED;
    else
        lv_pshared = PTHREAD_PROCESS_PRIVATE;
    int lv_status = pthread_spin_init(lp_sl, lv_pshared);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_init(%p) EXIT, ret=%d\n",
                     (void *) lp_sl, lv_status); // can't use static_cast
#endif
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
#ifdef SB_THREAD_LOCK_STATS
    iv_last_locker = 0;
#endif
}

#ifdef SB_THREAD_LINUX
SB_Thread::SL::~SL() {
    int lv_status = pthread_spin_destroy(&iv_sl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_destroy(%p) EXIT, ret=%d\n",
                     (void *) &iv_sl, lv_status); // can't use static_cast
#endif
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
}
#else
SB_Thread::SL::~SL() {
    pthread_spinlock_t *lp_sl;
    int                 lv_status;

    if (ip_sl != NULL) {
        lp_sl = static_cast<pthread_spinlock_t *>(ip_sl);
        lv_status = pthread_spin_destroy(lp_sl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
        if (gv_sb_trace_pthread)
            trace_printf("pthread_spin_destroy(%p) EXIT, ret=%d\n",
                         static_cast<void *>(lp_sl), lv_status);
#endif
        SB_util_assert_ieq(lv_status, 0); // sw fault
        lv_status = lv_status; // touch (in case assert disabled)
        delete lp_sl;
    }
}
#endif

// ----------------------------------------------------------------------------

#ifdef SB_THREAD_LINUX
SB_Thread::Usem::Usem() : iv_inited(false) {
}
#else
SB_Thread::Usem::Usem() {
}
#endif

#ifdef SB_THREAD_LINUX
SB_Thread::Usem::~Usem() {
    if (iv_inited) {
        int lv_status = sem_destroy(&iv_sem);
        SB_util_assert_ieq(lv_status, 0);
        lv_status = lv_status; // touch (in case assert disabled)
    }
}
#else
SB_Thread::Usem::~Usem() {
    sem_t *lp_sem;

    lp_sem = reinterpret_cast<sem_t *>(ip_sem);
    if (lp_sem != NULL) {
        int lv_status = sem_destroy(lp_sem);
        SB_util_assert_ieq(lv_status, 0);
        lv_status = lv_status; // touch (in case assert disabled)
        delete lp_sem;
    }
}
#endif

// ----------------------------------------------------------------------------

SB_Thread::Nsem::Nsem() : ip_sem(NULL) {
}

#ifdef SB_THREAD_LINUX
SB_Thread::Nsem::~Nsem() {
    if (ip_sem != NULL) {
        int lv_status = sem_close(ip_sem);
        SB_util_assert_ieq(lv_status, 0);
        lv_status = lv_status; // touch (in case assert disabled)
    }
}
#else
SB_Thread::Nsem::~Nsem() {
    sem_t *lp_sem;
    int    lv_status;

    lp_sem = reinterpret_cast<sem_t *>(ip_sem);
    if (lp_sem != NULL) {
        lv_status = sem_close(lp_sem);
        SB_util_assert_ieq(lv_status, 0);
        lv_status = lv_status; // touch (in case assert disabled)
    }
}
#endif

int SB_Thread::Nsem::init(const char   *pp_name,
                          int           pv_oflag,
                          unsigned int  pv_mode,
                          unsigned int  pv_value) {
    sem_t *lp_sem;
    int    lv_status;

    lp_sem = sem_open(pp_name, pv_oflag, pv_mode, pv_value);
    if (lp_sem == SEM_FAILED)
        lv_status = errno;
    else {
#ifdef SB_THREAD_LINUX
        ip_sem = lp_sem;
#else
        ip_sem = reinterpret_cast<Thr_Gen_Ptr>(lp_sem);
#endif
        lv_status = 0;
    }
    return lv_status;
}

