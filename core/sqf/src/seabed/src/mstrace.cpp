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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "seabed/trace.h"

#include "buf.h"
#include "env.h"
#include "msi.h"
#include "mstrace.h"
#include "msvars.h"
#include "msx.h"


void ms_help_bool(const char *pp_key, const char *pp_value,
                  const char *pp_key_cmp, bool *pp_value_ret) {
    if ((pp_key != NULL) && (strcasecmp(pp_key, pp_key_cmp) == 0))
        *pp_value_ret = atoi(pp_value);
}

void ms_help_int(const char *pp_key, const char *pp_value,
                 const char *pp_key_cmp, int *pp_value_ret) {
    if ((pp_key != NULL) && (strcasecmp(pp_key, pp_key_cmp) == 0))
        *pp_value_ret = atoi(pp_value);
}

void ms_help_longlong(const char *pp_key, const char *pp_value,
                      const char *pp_key_cmp, long long *pp_value_ret) {
    if ((pp_key != NULL) && (strcasecmp(pp_key, pp_key_cmp) == 0))
        *pp_value_ret = atoll(pp_value);
}

const char *ms_help_str(const char *pp_key, const char *pp_value,
                        const char *pp_key_cmp) {
    if ((pp_key != NULL) && (strcasecmp(pp_key, pp_key_cmp) == 0))
        return pp_value;
    return NULL;
}

void ms_trace_change_item(const char *pp_key, const char *pp_value) {
    const char *WHERE = "ms_trace_change_item";
    char        la_host[200];
    char        la_unique[200];
    const char *lp_p;
    bool        lv_old_enable;
    bool        lv_old_fb;
    bool        lv_old_inmem;

    // save interesting old variables
    lv_old_enable = gv_ms_trace_enable;
    lv_old_fb = gv_ms_trace_file_fb;
    lv_old_inmem = gv_ms_trace_file_inmem;

    // enable or disable trace?
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_enable, &gv_ms_trace_enable);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_file_delta, &gv_ms_trace_file_delta);
    ms_help_int(pp_key, pp_value,
                gp_ms_env_trace_file_fb, &gv_ms_trace_file_fb);
    ms_help_int(pp_key, pp_value,
                gp_ms_env_trace_file_inmem, &gv_ms_trace_file_inmem);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_file_signal, &gv_ms_trace_file_signal);
    if (gv_ms_trace_file_signal)
        ms_trace_signal_init();
    trace_set_assert_no_trace(gv_ms_assert_error);
    trace_set_delta(gv_ms_trace_file_delta);
    if (gv_ms_trace_file_nolock)
        trace_set_lock(!gv_ms_trace_file_nolock);
    // if now enabled, restart trace
    if ((!lv_old_enable) && gv_ms_trace_enable) {
        trace_init2(gp_ms_trace_file,
                    gv_ms_trace_file_unique,
                    gp_ms_trace_prefix,
                    false,
                    gv_ms_trace_file_maxsize);
        SB_Buf_Lline lv_cmdline;
        char *lp_s = SB_util_get_cmdline(0,
                                         true, // args
                                         &lv_cmdline,
                                         lv_cmdline.size());
        if (lp_s != NULL)
            trace_where_printf(WHERE, "cmdline='%s'\n", &lv_cmdline);

        ms_gather_info(WHERE);
    }
    // if now disabled, flush trace
    if (lv_old_enable && (!gv_ms_trace_enable)) {
        trace_where_printf(WHERE, "trace disabled\n");
        trace_flush();
    }
    // if new fb, set
    if ((gv_ms_trace_file_fb > 0) && (lv_old_fb != gv_ms_trace_file_fb))
        trace_set_mem(gv_ms_trace_file_fb);
    if ((gv_ms_trace_file_inmem > 0) &&
        (lv_old_inmem != gv_ms_trace_file_inmem))
        trace_set_inmem(gv_ms_trace_file_inmem);

    // set globals appropriately
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace, &gv_ms_trace);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_abandon, &gv_ms_trace_abandon);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_alloc, &gv_ms_trace_alloc);
    ms_buf_trace_change();
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_data, &gv_ms_trace_data);
    ms_help_int(pp_key, pp_value,
                gp_ms_env_trace_data_max, &gv_ms_trace_data_max);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_detail, &gv_ms_trace_detail);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_dialect, &gv_ms_trace_dialect);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_environ, &gv_ms_trace_environ);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_errors, &gv_ms_trace_errors);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_events, &gv_ms_trace_events);
    SB_Ms_Tl_Event_Mgr::set_trace_events(gv_ms_trace_events);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_evlog, &gv_ms_trace_evlog);
    lp_p = ms_help_str(pp_key, pp_value,
                       gp_ms_env_trace_file_dir);
    if (lp_p != NULL) {
        delete [] gp_ms_trace_file_dir;
        gp_ms_trace_file_dir = new char[strlen(lp_p) + 1];
        strcpy(gp_ms_trace_file_dir, lp_p);
    }
    ms_help_longlong(pp_key, pp_value,
                     gp_ms_env_trace_file_maxsize, &gv_ms_trace_file_maxsize);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_file_nolock, &gv_ms_trace_file_nolock);
    ms_help_int(pp_key, pp_value,
                gp_ms_env_trace_file_unique, &gv_ms_trace_file_unique);
    lp_p = ms_help_str(pp_key, pp_value,
                       gp_ms_env_trace_file);
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
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_locio, &gv_ms_trace_locio);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_md, &gv_ms_trace_md);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_mon, &gv_ms_trace_mon);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_name, &gv_ms_trace_name);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_params, &gv_ms_trace_params);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_pthread, &gv_sb_trace_pthread);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_ic, &gv_ms_trace_ic);
    lp_p = ms_help_str(pp_key, pp_value,
                       gp_ms_env_trace_prefix);
    if (lp_p != NULL) {
        delete [] gp_ms_trace_prefix;
        gp_ms_trace_prefix = new char[strlen(lp_p) + 1];
        strcpy(gp_ms_trace_prefix, lp_p);
    }
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_qalloc, &gv_ms_trace_qalloc);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_ref, &gv_ms_trace_ref);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_sm, &gv_ms_trace_sm);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_stats, &gv_ms_trace_stats);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_thread, &gv_sb_trace_thread);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_timer, &gv_ms_trace_timer);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_timermap, &gv_ms_trace_timermap);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_trans, &gv_ms_trace_trans);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_verbose, &gv_ms_trace_verbose);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_wait, &gv_ms_trace_wait);
    ms_help_int(pp_key, pp_value,
                gp_ms_env_trace_xx, &gv_ms_trace_xx);
    if (gv_ms_trace || gv_ms_trace_params)
        gv_ms_trace_errors = true;
}

//
// possible scenarios:
// 1. key=MS_TRACE_TIMER
//    value=1
// 2. key=MS_TRACE_TIMER
//    value=1,MS_TRACE_MON=1[...]
//
void ms_trace_change_list(const char *pp_key, const char *pp_value) {
    char  la_value[1024];
    char *lp_key;
    char *lp_p;
    char *lp_value;

    lp_p = strchr(const_cast<char *>(pp_value), ',');
    if (lp_p == NULL)
        ms_trace_change_item(pp_key, pp_value);
    else {
        ms_trace_change_item(gp_ms_env_trace_enable, "0");
        SB_util_assert_stlt(strlen(pp_value), sizeof(la_value));
        strcpy(la_value, pp_value); // make a copy
        lp_p = strchr(la_value, ',');
        *lp_p = 0; // substitute eol
        ms_trace_change_item(pp_key, la_value); // deal with 1st iter
        lp_key = &lp_p[1];
        while (*lp_key) {
            lp_p = strchr(lp_key, '=');
            if (lp_p == NULL)
                break;
            *lp_p = 0; // substitute eol
            lp_value = &lp_p[1];
            lp_p = strchr(lp_value, ',');
            if (lp_p != NULL)
                *lp_p = 0; // substitue eol
            ms_trace_change_item(lp_key, lp_value);
            if (lp_p == NULL)
                break;
            lp_key = &lp_p[1];
        }
        ms_trace_change_item(gp_ms_env_trace_enable, "1");
    }
}

extern "C" void ms_trace_off();
void ms_trace_off() {
    trace_flush();
}

extern "C" void ms_trace_on();
void ms_trace_on() {
    const char *WHERE = "ms_trace_on";

    trace_init2(gp_ms_trace_file,
                gv_ms_trace_file_unique,
                gp_ms_trace_prefix,
                false,
                gv_ms_trace_file_maxsize);
    SB_Buf_Lline lv_cmdline;
    char *lp_s = SB_util_get_cmdline(0,
                                     true, // args
                                     &lv_cmdline,
                                     lv_cmdline.size());
    if (lp_s != NULL)
        trace_where_printf(WHERE, "cmdline='%s'\n", &lv_cmdline);

    ms_gather_info(WHERE);
}

void ms_trace_signal(int pv_sig, siginfo_t *pp_info, void *pp_ctx) {
    pv_sig = pv_sig; // touch
    pp_info = pp_info; // touch
    pp_ctx = pp_ctx; // touch
    trace_flush();
}

void ms_trace_signal_init() {
    sigset_t lv_set;
    sigemptyset(&lv_set);
    sigaddset(&lv_set, SIGUSR2);
    struct sigaction lv_act;
    lv_act.sa_sigaction = ms_trace_signal;
    sigemptyset(&lv_act.sa_mask);
    lv_act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &lv_act, NULL);
}
