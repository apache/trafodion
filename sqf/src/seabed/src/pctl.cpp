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
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "apictr.h"
#include "chk.h"
#include "mserrmsg.h"
#include "mstrace.h"
#include "msutil.h"
#include "msx.h"
#include "phan.h"
#include "util.h"

// PROCESS_GETPAIRINFO_ parameter numbers
// defined in Guardian Procedure calls manual
enum {
    PGPI_PARAM_PHANDLE              =  1,
    PGPI_PARAM_PAIR                 =  2,
    PGPI_PARAM_MAXLEN               =  2,
    PGPI_PARAM_PAIR_LENGTH          =  3,
    PGPI_PARAM_PRIMARY_PHANDLE      =  4,
    PGPI_PARAM_BACKUP_PHANDLE       =  5,
    PGPI_PARAM_SEARCH               =  6,
    PGPI_PARAM_ANCESTOR_PHANDLE     =  7,
    PGPI_PARAM_NODE                 =  8,
    PGPI_PARAM_NODE_LEN             =  8,
    PGPI_PARAM_OPTIONS              =  9,
    PGPI_PARAM_ANCESTOR_DESC        = 10,
    PGPI_PARAM_ANCESTOR_DESC_MAXLEN = 10,
    PGPI_PARAM_ANCESTOR_DESC_LEN    = 11,
    PGPI_PARAM_ERROR_DETAIL         = 12
};

//
// external wakeup thread.
//
class Pctl_Ext_Wakeup_Thread : public SB_Thread::Thread {
public:
    static int   awake(int pv_pid, short pv_event);
    static void  create();
    static void  init();
    void         run();
    static void  shutdown();

private:
    Pctl_Ext_Wakeup_Thread();
    virtual     ~Pctl_Ext_Wakeup_Thread();

    static Pctl_Ext_Wakeup_Thread *cp_thread;
    static int                     cv_pid;
    static bool                    cv_shutdown;
    static int                     cv_sig;
};

static bool                  gv_pctl_external_wakeups    = false;
static SB_Phandle_Type       gv_pctl_phandle_null;
static bool                  gv_pctl_phandle_null_inited = false;

Pctl_Ext_Wakeup_Thread *Pctl_Ext_Wakeup_Thread::cp_thread   = NULL;
int                     Pctl_Ext_Wakeup_Thread::cv_pid      = -1;
bool                    Pctl_Ext_Wakeup_Thread::cv_shutdown = false;
int                     Pctl_Ext_Wakeup_Thread::cv_sig      = -1;

//
// forwards
//
static short XWAIT_com(short pv_mask, int pv_time, bool pv_residual);

static void *pctl_ext_wakeup_thread_fun(void *pp_arg) {
    Pctl_Ext_Wakeup_Thread *lp_thread =
      static_cast<Pctl_Ext_Wakeup_Thread *>(pp_arg);
    lp_thread->run();
    return NULL;
}

Pctl_Ext_Wakeup_Thread::Pctl_Ext_Wakeup_Thread()
: SB_Thread::Thread(pctl_ext_wakeup_thread_fun, "ext-wakeup-thread") {
    cv_pid = getpid();
}

Pctl_Ext_Wakeup_Thread::~Pctl_Ext_Wakeup_Thread() {
}

int Pctl_Ext_Wakeup_Thread::awake(int pv_pid, short pv_event) {
    int    lv_err;
    sigval lv_val;

    lv_val.sival_int = pv_event;
    lv_err = sigqueue(pv_pid, cv_sig, lv_val);
    return lv_err;
}

void Pctl_Ext_Wakeup_Thread::create() {
    if (cp_thread == NULL) {
        cp_thread = new Pctl_Ext_Wakeup_Thread();
        cp_thread->start();
    }
}

void Pctl_Ext_Wakeup_Thread::init() {
    int      lv_err;
    sigset_t lv_set;

    cv_sig = SIGRTMAX - 3;

    sigemptyset(&lv_set);
    sigaddset(&lv_set, cv_sig);
    lv_err = pthread_sigmask(SIG_BLOCK, &lv_set, NULL);
    SB_util_assert_ieq(lv_err, 0); // sw fault
}

void Pctl_Ext_Wakeup_Thread::run() {
    const char *WHERE = "Pctl_Ext_Wakeup_Thread::run";
    int         lv_err;
    siginfo_t   lv_info;
    sigset_t    lv_set;

    sigemptyset(&lv_set);
    sigaddset(&lv_set, cv_sig);

    while (!cv_shutdown) {
        lv_err = sigwaitinfo(&lv_set, &lv_info);
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "sigwait returned err=%d, sig=%d, val=%d\n",
                               lv_err, lv_info.si_signo, lv_info.si_int);
        if ((lv_err == -1) && (errno == EINTR))
            continue;
        SB_util_assert_ieq(lv_err, cv_sig);

        if (lv_info.si_int) {
            gv_ms_event_mgr.set_event_all(static_cast<short>(lv_info.si_int));
            continue;
        }

        if (cv_shutdown)
            break;
    }
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXITING ext wakeup thread\n");
}

void Pctl_Ext_Wakeup_Thread::shutdown() {
    const char *WHERE = "Pctl_Ext_Wakeup_Thread::shutdown";
    void       *lp_result;
    int         lv_err;
    int         lv_status;
    sigval      lv_val;

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "shutdown\n");
    cv_shutdown = true;
    if (cp_thread != NULL) {
        lv_val.sival_int = 0;
        lv_err = sigqueue(cv_pid, cv_sig, lv_val);
        SB_util_assert_ieq(lv_err, 0);
        lv_status = cp_thread->join(&lp_result);
        SB_util_assert_ieq(lv_status, 0);
        delete cp_thread;
    }
    cp_thread = NULL;
}

void pctl_init() {
    Pctl_Ext_Wakeup_Thread::init();
}

void pctl_shutdown() {
    Pctl_Ext_Wakeup_Thread::shutdown();
}

static void pctl_util_init_phandle_null() {
    int lv_status;

    gv_pctl_phandle_null_inited = true;
    lv_status = XPROCESSHANDLE_NULLIT_(&gv_pctl_phandle_null);
    CHK_FEIGNORE(lv_status);
}

//
// Purpose: allow external wakeups
//
SB_Export void proc_enable_external_wakeups() {
    const char *WHERE = "proc_enable_external_wakeups";
    SB_API_CTR (lv_zctr, PROC_ENABLE_EXTERNAL_WAKEUPS);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    if (!gv_pctl_external_wakeups)
        Pctl_Ext_Wakeup_Thread::create();
    gv_pctl_external_wakeups = true;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

#ifdef USE_EVENT_REG
//
// Purpose: deregister event
//
SB_Export int proc_event_deregister(short pv_event) {
    const char *WHERE = "proc_event_deregister";
    short       lv_fserr;
    SB_API_CTR (lv_zctr, PROC_EVENT_DEREGISTER);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER event=0x%x\n", pv_event);
    switch (pv_event) {
    case LREQ:
        lv_fserr = XZFIL_ERR_OK;
        break;
    case LDONE:
        lv_fserr = XZFIL_ERR_OK;
        break;
    default:
        lv_fserr = XZFIL_ERR_INVALOP;
    }

    if (lv_fserr == XZFIL_ERR_OK)
        gv_ms_event_mgr.get_mgr(NULL)->deregister_event(pv_event);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}

//
// Purpose: disable event abort
//
SB_Export void proc_event_disable_abort() {
    SB_API_CTR (lv_zctr, PROC_EVENT_DISABLE_ABORT);
    gv_ms_event_wait_abort = false;
}

//
// Purpose: register event
//
SB_Export int proc_event_register(short pv_event) {
    const char *WHERE = "proc_event_register";
    short       lv_fserr;
    SB_API_CTR (lv_zctr, PROC_EVENT_REGISTER);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE,
                           "ENTER event=0x%x\n", pv_event);
    switch (pv_event) {
    case LREQ:
        lv_fserr = XZFIL_ERR_OK;
        break;
    case LDONE:
        lv_fserr = XZFIL_ERR_OK;
        break;
    default:
        lv_fserr = XZFIL_ERR_INVALOP;
    }

    if (lv_fserr == XZFIL_ERR_OK)
        if (!gv_ms_event_mgr.get_mgr(NULL)->register_event(pv_event))
            lv_fserr = XZFIL_ERR_TOOMANY;

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(lv_fserr);
}
#endif // USE_EVENT_REG

SB_Export void proc_register_group_pin(int pv_group, int pv_pin) {
    SB_API_CTR (lv_zctr, PROC_REGISTER_GROUP_PIN);
    gv_ms_event_mgr.get_mgr(NULL)->register_group_pin(pv_group, pv_pin);
}

SB_Export int proc_set_process_completion() {
    const char *WHERE = "proc_set_process_completion";
    int         lv_fserr;
    SB_API_CTR (lv_zctr, PROC_SET_PROCESS_COMPLETION);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    if (!gv_ms_calls_ok)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    gv_ms_process_comp = true;
#ifndef USE_EVENT_REG
    gv_ms_event_mgr.set_ret_gmgr(true);
#endif
    lv_fserr = XZFIL_ERR_OK;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return ms_err_rtn(static_cast<short>(lv_fserr));
}

//
// Purpose: emulate AWAKE
//
SB_Export void XAWAKE(int pv_pin, short pv_event) {
    const char *WHERE = "XAWAKE";
    SB_API_CTR (lv_zctr, XAWAKE);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER pin=%d, event=%d\n", pv_pin, pv_event);
    if (!gv_ms_calls_ok) {
        ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                       XZFIL_ERR_INVALIDSTATE);
        return;
    }

    gv_ms_event_mgr.get_mgr(NULL)->set_event_pin(pv_pin, pv_event, 0);
}

//
// Purpose: emulate AWAKE_A06
//
// pv_fun: 0 - wake up single process
//         1 - wake up whole group
//         2 - wake up until one was waiting
//
SB_Export void XAWAKE_A06(int pv_pin, short pv_event, short pv_fun) {
    const char *WHERE = "XAWAKE_A06";
    SB_API_CTR (lv_zctr, XAWAKE_A06);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER pin=%d, event=%d, fun=%d\n",
                           pv_pin, pv_event, pv_fun);
    if (!gv_ms_calls_ok) {
        ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                       XZFIL_ERR_INVALIDSTATE);
        return;
    }

    gv_ms_event_mgr.get_mgr(NULL)->set_event_pin(pv_pin, pv_event, pv_fun);
}

SB_Export short XPROCESS_AWAKE_(int pv_pid, short pv_event) {
    const char *WHERE = "XPROCESS_AWAKE_";
    int         lv_err;
    short       lv_fserr;
    SB_API_CTR (lv_zctr, XPROCESS_AWAKE_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER pid=%d, event=%d\n", pv_pid, pv_event);

    if (!gv_ms_calls_ok)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);

    lv_err = Pctl_Ext_Wakeup_Thread::awake(pv_pid, pv_event);
    if (lv_err) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "err=%d, errno=%d\n", lv_err, errno);
        switch (errno) {
        case EAGAIN: // limit of signals which may be queued reached
            lv_fserr = XZFIL_ERR_TOOMANY;
            break;
        case EINVAL: // sig was invalid
            lv_fserr = XZFIL_ERR_FSERR;
            break;
        case EPERM: // does not have permission to send signal
            lv_fserr = XZFIL_ERR_FSERR;
            break;
        case ESRCH: // no process has a matching pid
            lv_fserr = XZFIL_ERR_NOSUCHDEV;
            break;
        default:
            lv_fserr = XZFIL_ERR_FSERR;
            break;
        }
    } else
        lv_fserr = XZFIL_ERR_OK;
    return ms_err_rtn_msg(WHERE, "EXIT", lv_fserr);
}

SB_Export short XPROCESS_GETPAIRINFO_(SB_Phandle_Type *pp_phandle,
                                      char            *pp_pair,
                                      short            pv_maxlen,
                                      short           *pp_pair_length,
                                      SB_Phandle_Type *pp_primary_phandle,
                                      SB_Phandle_Type *pp_backup_phandle,
                                      int             *pp_search,
                                      SB_Phandle_Type *pp_ancestor_phandle,
                                      char            *pp_node,
                                      short            pv_node_len,
                                      short            pv_options,
                                      char            *pp_ancestor_desc,
                                      short            pv_ancestor_desc_maxlen,
                                      short           *pp_ancestor_desc_len,
                                      short           *pp_error_detail) {
    const char               *WHERE = "XPROCESS_GETPAIRINFO_";
    char                      la_name[MS_MON_MAX_PROCESS_NAME];
    char                     *lp_name;
    short                    *lp_phandles;
    int                       lv_fserr;
    short                     lv_param;
    SB_Phandle_Type           lv_phandle;
    bool                      lv_phandle_null;
    MS_Mon_Process_Info_Type  lv_pi;
    bool                      lv_pi_get;
    bool                      lv_pi_ok;
    short                     lv_ret;
    int                       lv_status;
    SB_API_CTR               (lv_zctr, XPROCESS_GETPAIRINFO_);

    lv_pi_get = true;
    lv_pi_ok = false;
    lp_phandles = reinterpret_cast<short *>(pp_phandle);
    if ((pp_phandle == NULL) || (lp_phandles[0] == -1))
        lv_phandle_null = true;
    else
        lv_phandle_null = false;
    if (gv_ms_trace_params) {
        char la_phandlet[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandlet, pp_phandle);
        trace_where_printf(WHERE, "ENTER phandle=%s, pair=%p, maxlen=%d, pair-length=%p, p-phandle=%p, b-phandle=%p\n",
                           la_phandlet,
                           pfp(pp_pair),
                           pv_maxlen,
                           pfp(pp_pair_length),
                           pfp(pp_primary_phandle),
                           pfp(pp_backup_phandle));
        trace_where_printf(WHERE, "search=%p(%d), a-phandle=%p, node=%p, nodelen=%d, options=%d, a-desc=%p, a-desc-maxlen=%d, a-desc-len=%p, err-detail=%p\n",
                           pfp(pp_search),
                           (pp_search != NULL) ? *pp_search : -1,
                           pfp(pp_ancestor_phandle),
                           pfp(pp_node),
                           pv_node_len,
                           pv_options,
                           pfp(pp_ancestor_desc),
                           pv_ancestor_desc_maxlen,
                           pfp(pp_ancestor_desc_len),
                           pfp(pp_error_detail));
    }
    if (!gv_ms_calls_ok) {
        lv_ret = XPROC_INVSTATE;
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              lv_ret);
    }
    lv_ret = XPROC_OK;
    lv_fserr = XZFIL_ERR_OK;
    if (pp_error_detail != NULL)
        *pp_error_detail = 0;

    // check 'legacy' parameters and reject
    if ((pp_search != NULL) && (*pp_search != -1)) {
        lv_ret = XPROC_PRM_UNSUP;
        lv_param = PGPI_PARAM_SEARCH;
        if (pp_error_detail != NULL)
            *pp_error_detail = lv_param;
        return ms_err_rtn_msg(WHERE, "search (non -1) parameter unsupported", lv_ret);
    }
    if (pp_ancestor_phandle != NULL) {
        lv_ret = XPROC_PRM_UNSUP;
        lv_param = PGPI_PARAM_ANCESTOR_PHANDLE;
        if (pp_error_detail != NULL)
            *pp_error_detail = lv_param;
        return ms_err_rtn_msg(WHERE, "ancestor-phandle parameter unsupported", lv_ret);
    }
    if ((pp_node != NULL) && pv_node_len) {
        lv_ret = XPROC_PRM_UNSUP;
        lv_param = PGPI_PARAM_NODE;
        if (pp_error_detail != NULL)
            *pp_error_detail = lv_param;
        return ms_err_rtn_msg(WHERE, "node/len parameter unsupported", lv_ret);
    }
    if (pv_options) {
        lv_ret = XPROC_PRM_UNSUP;
        lv_param = PGPI_PARAM_OPTIONS;
        if (pp_error_detail != NULL)
            *pp_error_detail = lv_param;
        return ms_err_rtn_msg(WHERE, "options (non-zero) parameter unsupported", lv_ret);
    }
    if ((pp_ancestor_desc != NULL) && pv_ancestor_desc_maxlen) {
        lv_ret = XPROC_PRM_UNSUP;
        lv_param = PGPI_PARAM_ANCESTOR_DESC;
        if (pp_error_detail != NULL)
            *pp_error_detail = lv_param;
        return ms_err_rtn_msg(WHERE, "ancestor-desc/maxlen parameter unsupported", lv_ret);
    }

    if (lv_phandle_null) {
        // null phandle, check pair param
        if (pp_pair_length == NULL) { // in
            // use pair name to create phandle
            if (pp_pair == NULL) {
                lv_ret = XPROC_BNDS;
                lv_param = PGPI_PARAM_PAIR;
                if (pp_error_detail != NULL)
                    *pp_error_detail = lv_param;
                return ms_err_rtn_msg(WHERE, "invalid pair", lv_ret);
            }
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "pair=%s\n", pp_pair);
            lp_name = la_name;
            memcpy(la_name, pp_pair, sbmin(pv_maxlen, static_cast<int>(sizeof(la_name))));
            if (pv_maxlen < static_cast<int>(sizeof(la_name)))
                la_name[pv_maxlen] = '\0';
            else
                la_name[MS_MON_MAX_PROCESS_NAME-1] = '\0';
            lv_pi_get = false;
            lv_fserr = msg_mon_get_process_info_detail(la_name, &lv_pi);
            if (lv_fserr == XZFIL_ERR_OK) {
                ms_util_fill_phandle_name(&lv_phandle,
                                          la_name,
                                          lv_pi.nid,
                                          lv_pi.pid
#ifdef SQ_PHANDLE_VERIFIER
                                         ,lv_pi.verifier
#endif
                                         );
                lv_pi_ok = true;
            } else if (lv_fserr == XZFIL_ERR_NOTFOUND)
                lv_ret = XPROC_NONEXTANT;
            else
                lv_ret = XPROC_NODEDOWN;
        } else { // out
            // use our phandle and return pair
            lv_fserr = XPROCESSHANDLE_GETMINE_(&lv_phandle);
            lp_name = ms_od_map_phandle_to_name(&lv_phandle);
            if ((pp_pair != NULL) && (lv_fserr == XZFIL_ERR_OK)) {
                *pp_pair_length = static_cast<short>(strlen(lp_name));
                memcpy(pp_pair, lp_name, *pp_pair_length);
                if (gv_ms_trace_params)
                    trace_where_printf(WHERE, "pair=%s\n", lp_name);
            }
            if (lv_fserr != XZFIL_ERR_OK)
                lv_ret = XPROC_INVSTATE;
        }
    } else {
        lp_name = ms_od_map_phandle_to_name(pp_phandle);
        lv_pi_get = false;
        lv_fserr = msg_mon_get_process_info_detail(lp_name, &lv_pi);
        if (lv_fserr == XZFIL_ERR_OK) {
            lv_pi_ok = true;
            ms_util_fill_phandle_name(&lv_phandle,
                                      lp_name,
                                      lv_pi.nid,
                                      lv_pi.pid
#ifdef SQ_PHANDLE_VERIFIER
                                     ,lv_pi.verifier
#endif
                                     );
            if (pp_pair_length != NULL) { // out
                *pp_pair_length = static_cast<short>(strlen(lp_name));
                memcpy(pp_pair, lp_name, *pp_pair_length);
                if (gv_ms_trace_params)
                    trace_where_printf(WHERE, "pair=%s\n", lp_name);
            }
        } else if (lv_fserr == XZFIL_ERR_NOTFOUND)
            lv_ret = XPROC_NONEXTANT;
        else
            lv_ret = XPROC_NODEDOWN;
    }

    // at this point, name pair should be setup
    if (lv_fserr == XZFIL_ERR_OK) {
        if (lv_pi_get)
            lv_fserr = msg_mon_get_process_info_detail(lp_name, &lv_pi);
        if (lv_fserr == XZFIL_ERR_OK) {
            lv_pi_ok = true;
            if (pp_primary_phandle != NULL) {
                ms_util_fill_phandle_name(pp_primary_phandle,
                                          lp_name,
                                          !lv_pi.backup ? lv_pi.nid : lv_pi.parent_nid,
                                          !lv_pi.backup ? lv_pi.pid : lv_pi.parent_pid
#ifdef SQ_PHANDLE_VERIFIER
                                         ,!lv_pi.backup ? lv_pi.verifier : lv_pi.parent_verifier
#endif
                                         );
                if (gv_ms_trace_params) {
                    char la_phandlet[MSG_UTIL_PHANDLE_LEN];
                    msg_util_format_phandle(la_phandlet, pp_primary_phandle);
                    trace_where_printf(WHERE, "p-phandle=%s\n", la_phandlet);
                }
            }
            if (pp_backup_phandle != NULL) {
                if (strcmp(lv_pi.parent_name, lv_pi.process_name) == 0)
                    ms_util_fill_phandle_name(pp_backup_phandle,
                                              lp_name,
                                              !lv_pi.backup ? lv_pi.parent_nid : lv_pi.nid,
                                              !lv_pi.backup ? lv_pi.parent_pid : lv_pi.pid
#ifdef SQ_PHANDLE_VERIFIER
                                             ,!lv_pi.backup ? lv_pi.parent_verifier : lv_pi.verifier
#endif
                                             );
                else {
                    lv_status = XPROCESSHANDLE_NULLIT_(pp_backup_phandle);
                    CHK_FEIGNORE(lv_status);
                }
                if (gv_ms_trace_params) {
                    char la_phandlet[MSG_UTIL_PHANDLE_LEN];
                    msg_util_format_phandle(la_phandlet, pp_backup_phandle);
                    trace_where_printf(WHERE, "b-phandle=%s\n", la_phandlet);
                }
            }
        } else if (lv_fserr == XZFIL_ERR_NOTFOUND)
            lv_ret = XPROC_NONEXTANT;
        else
            lv_ret = XPROC_NODEDOWN;
    }

    if (lv_pi_ok) {
        if (strcmp(lv_pi.parent_name, lv_pi.process_name) == 0) {
            // process-pair
            if (!lv_pi.backup) {
                if ((lv_pi.nid == gv_ms_su_nid) &&
                    (lv_pi.pid == gv_ms_su_pid))
                    lv_ret = XPROC_PRIMARY;
                else if ((lv_pi.parent_nid == gv_ms_su_nid) &&
                         (lv_pi.parent_pid == gv_ms_su_pid))
                    lv_ret = XPROC_BACKUP;
                else
                    lv_ret = XPROC_OK; // not the calling process
            } else {
                if ((lv_pi.nid == gv_ms_su_nid) &&
                    (lv_pi.pid == gv_ms_su_pid))
                    lv_ret = XPROC_BACKUP;
                else if ((lv_pi.parent_nid == gv_ms_su_nid) &&
                         (lv_pi.parent_pid == gv_ms_su_pid))
                    lv_ret = XPROC_PRIMARY;
                else
                    lv_ret = XPROC_OK; // not the calling process
            }
        } else
            lv_ret = XPROC_SINGLE;
    }

    return ms_err_rtn_msg_noassert(WHERE, "EXIT", lv_ret);
}

SB_Export short XPROCESSHANDLE_DECOMPOSE_(SB_Phandle_Type *pp_phandle,
                                          int             *pp_cpu,
                                          int             *pp_pin,
                                          int             *pp_nodenumber,
                                          char            *pp_nodename,
                                          short            pv_nodename_maxlen,
                                          short           *pp_nodename_length,
                                          char            *pp_procname,
                                          short            pv_procname_maxlen,
                                          short           *pp_procname_length,
                                          SB_Int64_Type   *pp_sequence_number) {
    const char  *WHERE = "XPROCESSHANDLE_DECOMPOSE_";
    SB_Phandle  *lp_phandle = reinterpret_cast<SB_Phandle *>(pp_phandle);
    short       *lp_phandles = reinterpret_cast<short *>(pp_phandle);
    int          lv_nodelen;
    int          lv_proclen;
    SB_API_CTR  (lv_zctr, XPROCESSHANDLE_DECOMPOSE_);

    if (gv_ms_trace_params) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, pp_phandle);
        trace_where_printf(WHERE, "ENTER phandle=%s, cpu=%p, pin=%p, nodenum=%p, nodename=%p, nodename_max=%d, nodename_len=%p, pname=%p, pname_max=%d, pname_len=%p, seq=%p\n",
                           la_phandle,
                           pfp(pp_cpu),
                           pfp(pp_pin),
                           pfp(pp_nodenumber),
                           pp_nodename,
                           pv_nodename_maxlen,
                           pfp(pp_nodename_length),
                           pp_procname,
                           pv_procname_maxlen,
                           pfp(pp_procname_length),
                           pfp(pp_sequence_number));
    }
    if (!gv_ms_calls_ok)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_phandle == NULL)
        return ms_err_rtn_msg(WHERE, "invalid phandle (null)", XZFIL_ERR_BOUNDSERR);
    if (lp_phandles[0] == -1)
        return ms_err_rtn_msg(WHERE, "invalid phandle (null-phandle)", XZFIL_ERR_BADPARMVALUE);
    if (lp_phandle->iv_type != PH_NAMED)
        return ms_err_rtn_msg(WHERE, "invalid phandle", XZFIL_ERR_BADPARMVALUE);
    if (pp_cpu != NULL) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "cpu=%p(%d)\n",
                               pfp(pp_cpu),
                               lp_phandle->iv_nid);
        *pp_cpu = lp_phandle->iv_nid;
    }
    if (pp_pin != NULL) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "pin=%p(%d)\n",
                               pfp(pp_pin),
                               lp_phandle->iv_pid);
        *pp_pin = lp_phandle->iv_pid;
    }
    if (pp_nodenumber != NULL) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "nodenumber=%p(0)\n", pfp(pp_nodenumber));
        *pp_nodenumber = 0;
    }
    if (pp_nodename != NULL) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "nodename=%p(%s)\n",
                               pp_nodename, "\\NSK");
        lv_nodelen = sbmin(4, pv_nodename_maxlen);
        memcpy(pp_nodename, "\\NSK", lv_nodelen);
        if (pp_nodename_length != NULL)
            *pp_nodename_length = static_cast<short>(lv_nodelen);
    }
    if (pp_procname != NULL) {
        lv_proclen = static_cast<int>(lp_phandle->iv_name_len);
        lv_proclen = sbmin(lv_proclen, pv_procname_maxlen);
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "pname=%p(%s), pname_len=%d\n",
                               pfp(pp_procname),
                               lp_phandle->ia_name,
                               lv_proclen);
        memcpy(pp_procname, lp_phandle->ia_name, lv_proclen);
        if (pp_procname_length != NULL)
            *pp_procname_length = static_cast<short>(lv_proclen);
    }
    if (pp_sequence_number != NULL) {
#ifdef SQ_PHANDLE_VERIFIER
        *pp_sequence_number = lp_phandle->iv_verifier;
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "sequence_number=%p(" PF64 ")\n",
                               pfp(pp_sequence_number), *pp_sequence_number);
#else
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "sequence_number=%p(0)\n",
                               pfp(pp_sequence_number));
        *pp_sequence_number = 0;
#endif
    }
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT OK\n");
    return XZFIL_ERR_OK;
}

SB_Export short XPROCESSHANDLE_COMPARE_(SB_Phandle_Type *pp_phandle1,
                                        SB_Phandle_Type *pp_phandle2) {
    const char *WHERE = "XPROCESSHANDLE_COMPARE_";
    enum {
        RET_UNRELATED = 0,
        RET_PAIR      = 1,
        RET_IDENTICAL = 2
    };
    const char *lp_ret_text;
    int         lv_ret;
    SB_API_CTR (lv_zctr, XPROCESSHANDLE_COMPARE_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER phandle1=%p, phandle2=%p\n",
                           pfp(pp_phandle1), pfp(pp_phandle2));
    if (pp_phandle1 == NULL) {
        if (!gv_pctl_phandle_null_inited)
            pctl_util_init_phandle_null();
        pp_phandle1 = &gv_pctl_phandle_null;
    }
    if (pp_phandle2 == NULL) {
        if (!gv_pctl_phandle_null_inited)
            pctl_util_init_phandle_null();
        pp_phandle2 = &gv_pctl_phandle_null;
    }
    SB_Phandle *lp_phandle1 = reinterpret_cast<SB_Phandle *>(pp_phandle1);
    SB_Phandle *lp_phandle2 = reinterpret_cast<SB_Phandle *>(pp_phandle2);
    if ((lp_phandle1->iv_type == PH_NULL) &&
        (lp_phandle2->iv_type == PH_NULL)) {
        lp_ret_text = "IDENTICAL, both null";
        lv_ret = RET_IDENTICAL;
    } else if ((lp_phandle1->iv_type == PH_NAMED) &&
               (lp_phandle2->iv_type == PH_NAMED)) {
        if (strcmp(lp_phandle1->ia_name, lp_phandle2->ia_name) == 0) {
#ifdef SQ_PHANDLE_VERIFIER
            if ((lp_phandle1->iv_nid == lp_phandle2->iv_nid) &&
                (lp_phandle1->iv_pid == lp_phandle2->iv_pid) &&
                (lp_phandle1->iv_verifier == lp_phandle2->iv_verifier)) {
                lp_ret_text = "IDENTICAL, name, nid/pid/verifier match";
                lv_ret = RET_IDENTICAL;
            } else {
                lp_ret_text = "PAIR, names match, nid/pid/verifier do not match";
                lv_ret = RET_PAIR;
            }
#else
            if ((lp_phandle1->iv_nid == lp_phandle2->iv_nid) &&
                (lp_phandle1->iv_pid == lp_phandle2->iv_pid)) {
                lp_ret_text = "IDENTICAL, name, nid/pid match";
                lv_ret = RET_IDENTICAL;
            } else {
                lp_ret_text = "PAIR, names match, nid/pid do not match";
                lv_ret = RET_PAIR;
            }
#endif
        } else {
            lp_ret_text = "UNRELATED, names do not match";
            lv_ret = RET_UNRELATED;
        }
    } else {
        lp_ret_text = "UNRELATED, not both named or not both null";
        lv_ret = RET_UNRELATED;
    }
    if (gv_ms_trace_params) {
        char la_phandle1[MSG_UTIL_PHANDLE_LEN];
        char la_phandle2[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle1, pp_phandle1);
        msg_util_format_phandle(la_phandle2, pp_phandle2);
        trace_where_printf(WHERE, "EXIT %s, phandle1=%s, phandle2=%s\n",
                           lp_ret_text, la_phandle1, la_phandle2);
    }
    return static_cast<short>(lv_ret);
}

SB_Export short XPROCESSHANDLE_GETMINE_(SB_Phandle_Type *pp_phandle) {
    const char *WHERE = "XPROCESSHANDLE_GETMINE_";
    SB_API_CTR (lv_zctr, XPROCESSHANDLE_GETMINE_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER phandle=%p\n", pfp(pp_phandle));
    if (!gv_ms_calls_ok)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    ms_od_get_my_phandle(pp_phandle);
    if (gv_ms_trace_params) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        msg_util_format_phandle(la_phandle, pp_phandle);
        trace_where_printf(WHERE, "EXIT OK, phandle=%s\n", la_phandle);
    }
    return XZFIL_ERR_OK;
}

SB_Export short XPROCESSHANDLE_NULLIT_(SB_Phandle_Type *pp_phandle) {
    const char *WHERE = "XPROCESSHANDLE_NULLIT_";
    SB_API_CTR (lv_zctr, XPROCESSHANDLE_NULLIT_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER phandle=%p\n", pfp(pp_phandle));
    if (!gv_ms_calls_ok)
        return ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown",
                              XZFIL_ERR_INVALIDSTATE);
    if (pp_phandle == NULL)
        return ms_err_rtn_msg(WHERE, "invalid phandle", XZFIL_ERR_BOUNDSERR);
    for (int lv_inx = 0; lv_inx < SB_PHANDLE_LL_SIZE; lv_inx++)
        pp_phandle->_data[lv_inx] = -1;
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT OK\n");
    return XZFIL_ERR_OK;
}

//
// Purpose: emulate WAIT
//
// timeout:
//   -2 check for event (don't wait)
//   -1 wait forever
//   >0 max time to wait in 10 ms units
//
SB_Export short XWAIT(short pv_mask, int pv_time) {
    SB_API_CTR (lv_zctr, XWAIT);
    // TODO: residual to be changed to false
    return XWAIT_com(pv_mask, pv_time, true);
}

//
// Purpose: emulate WAIT
//
// timeout:
//   -2 check for event (don't wait)
//   -1 wait forever
//    0 use residual time value
//   >0 max time to wait in 10 ms units
//
SB_Export short XWAIT0(short pv_mask, int pv_time) {
    SB_API_CTR (lv_zctr, XWAIT0);
    return XWAIT_com(pv_mask, pv_time, true);
}

//
// Purpose: emulate WAIT (except no zero)
//
// timeout:
//   -2 check for event (don't wait)
//   -1 wait forever
//   >0 max time to wait in 10 ms units
//
SB_Export short XWAITNO0(short pv_mask, int pv_time) {
    SB_API_CTR (lv_zctr, XWAITNO0);
    return XWAIT_com(pv_mask, pv_time, false);
}

//
// Purpose: emulate WAIT
//
//
short XWAIT_com(short pv_mask, int pv_time, bool pv_residual) {
    const char    *WHERE = "XWAIT";
    SB_Int64_Type  lv_curr_time;

    if (gv_ms_trace_wait)
        trace_where_printf(WHERE, "ENTER mask=%d(0x%x), time=%d\n",
                           pv_mask, pv_mask, pv_time);
    if (!gv_ms_calls_ok) {
        ms_err_rtn_msg(WHERE, "msg_init() not called or shutdown", XZFIL_ERR_INVALIDSTATE);
        return 0;
    }

    // wait order LCAN, LDONE, LTMF, LREQ
    SB_util_assert_ige(pv_time, -2); // sw fault

    if (!pv_residual)
        SB_util_assert_ine(pv_time, 0); // sw fault

    int lv_events;
    long lv_to;
    bool lv_first;
    SB_Ms_Event_Mgr *lp_mgr = gv_ms_event_mgr.get_mgr(&lv_first);
    // If first time, set LREQ.
    // This is in case something was received before XWAIT called.
    // The application may see spurious LREQ.
    if (lv_first) {
        lp_mgr->set_event(LREQ, NULL);
        if (gv_ms_trace_wait)
            trace_where_printf(WHERE, "setting LREQ\n");
    }
#ifdef USE_EVENT_REG
    if (gv_ms_event_wait_abort) {
        if (pv_mask & LREQ) {
            if (!lp_mgr->register_event_check(LREQ)) {
                char *lp_str = "XWAIT called with LREQ without proc_event_register(LREQ)\n";
                if (gv_ms_trace_wait)
                    trace_where_printf(WHERE, lp_str);
                fprintf(stderr, lp_str);
                SB_util_assert_bt(lp_mgr->register_event_check(LREQ));
            }
        }
        if (pv_mask & LDONE) {
            if (!lp_mgr->register_event_check(LDONE)) {
                char *lp_str = "XWAIT called with LDONE without proc_event_register(LDONE)\n";
                if (gv_ms_trace_wait)
                    trace_where_printf(WHERE, lp_str);
                fprintf(stderr, lp_str);
                SB_util_assert_bt(lp_mgr->register_event_check(LDONE));
            }
        }
    }
#endif // USE_EVENT_REG
    // lv_curr_time contains cached current time
    // so that getting the current-time is minimized
    lp_mgr->set_wait_start_time(pv_time, &lv_curr_time);
    lv_to = lp_mgr->get_wait_time_start(pv_time);
    bool lv_check = (lv_to == -2);
    lv_first = true;
    do {
        lv_events = lp_mgr->get_event(pv_mask);
        if (lv_events)
            break;
        if (lv_check)
            break;
        if (lp_mgr->get_timedout(&lv_curr_time))
            break;
        if (lv_first)
            lv_first = false;
        else
            lv_to = lp_mgr->get_wait_time_rem(lv_curr_time);
        lp_mgr->wait(lv_to);
        lv_curr_time = 0;
    } while (!lv_events);
    if (pv_residual)
        lp_mgr->set_wait_time(pv_time);
    if (gv_ms_trace_wait)
        trace_where_printf(WHERE, "EXIT events=%d(0x%x)\n",
                           lv_events, lv_events);
    return static_cast<short>(lv_events);
}

