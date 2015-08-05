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

#include "seabed/int/opts.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/trace.h"

#include "buf.h"
#include "env.h"
#include "fsi.h"
#include "fstrace.h"
#include "mstrace.h"
#include "msx.h"

// globals
char       *gp_fs_trace_file         = NULL;
char       *gp_fs_trace_file_dir     = NULL;
char       *gp_fs_trace_prefix       = const_cast<char *>("FS");
bool        gv_fs_trace              = false;
bool        gv_fs_trace_data         = false;
int         gv_fs_trace_data_max     = -1;
bool        gv_fs_trace_detail       = false;
bool        gv_fs_trace_enable       = false;
bool        gv_fs_trace_errors       = false;
bool        gv_fs_trace_file_delta   = false;
int         gv_fs_trace_file_fb      = 0;
long long   gv_fs_trace_file_maxsize = INT_MAX;
bool        gv_fs_trace_file_nolock  = false;
int         gv_fs_trace_file_unique  = 0;
bool        gv_fs_trace_mon          = false;
bool        gv_fs_trace_mt           = false;
bool        gv_fs_trace_params       = false;
bool        gv_fs_trace_params0      = false;
bool        gv_fs_trace_verbose      = false;

void fs_trace_change_item(const char *pp_key, const char *pp_value) {
    const char *WHERE = "fs_trace_change_item";
    char        la_host[200];
    char        la_unique[200];
    const char *lp_p;
    bool        lv_old_enable;
    bool        lv_old_fb;

    // save interesting old variables
    lv_old_enable = gv_fs_trace_enable;
    lv_old_fb = gv_fs_trace_file_fb;

    // set globals appropriately
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_enable, &gv_fs_trace_enable);
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace, &gv_fs_trace);
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_data, &gv_fs_trace_data);
    ms_help_int(pp_key, pp_value,
                 gp_fs_env_trace_data_max, &gv_fs_trace_data_max);
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_detail, &gv_fs_trace_detail);
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_errors, &gv_fs_trace_errors);
    lp_p = ms_help_str(pp_key, pp_value,
                       gp_fs_env_trace_file_dir);
    if (lp_p != NULL) {
        delete [] gp_fs_trace_file_dir;
        gp_fs_trace_file_dir = new char[strlen(lp_p) + 1];
        strcpy(gp_fs_trace_file_dir, lp_p);
    }
    ms_help_longlong(pp_key, pp_value,
                     gp_fs_env_trace_file_maxsize, &gv_fs_trace_file_maxsize);
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_file_nolock, &gv_ms_trace_file_nolock);
    ms_help_int(pp_key, pp_value,
                gp_fs_env_trace_file_unique, &gv_fs_trace_file_unique);
    lp_p = ms_help_str(pp_key, pp_value,
                       gp_fs_env_trace_file);
    if (lp_p != NULL) {
        delete [] gp_fs_trace_file;
        if (gv_fs_trace_file_unique < 0) {
            gethostname(la_host, sizeof(la_host));
            sprintf(la_unique, "%s.%s.", lp_p, la_host);
            lp_p = la_unique;
        }
        if (gp_fs_trace_file_dir == NULL) {
            gp_fs_trace_file = new char[strlen(lp_p) + 1];
            strcpy(gp_fs_trace_file, lp_p);
        } else {
            gp_fs_trace_file =
              new char[strlen(gp_fs_trace_file_dir) + strlen(lp_p) + 2];
            sprintf(gp_fs_trace_file, "%s/%s", gp_fs_trace_file_dir, lp_p);
        }
    }
    ms_help_bool(pp_key, pp_value,
                 gp_ms_env_trace_file_delta, &gv_ms_trace_file_delta);
    ms_help_int(pp_key, pp_value,
                 gp_fs_env_trace_file_fb, &gv_fs_trace_file_fb);
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_mon, &gv_fs_trace_mon);
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_mt, &gv_fs_trace_mt);
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_params, &gv_fs_trace_params);
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_params0, &gv_fs_trace_params0);
    lp_p = ms_help_str(pp_key, pp_value,
                       gp_fs_env_trace_prefix);
    if (lp_p != NULL) {
        delete [] gp_fs_trace_prefix;
        gp_fs_trace_prefix = new char[strlen(lp_p) + 1];
        strcpy(gp_fs_trace_prefix, lp_p);
    }
    if ((gv_fs_trace || gv_fs_trace_params) && !gv_fs_trace_params0)
        gv_fs_trace_errors = true;
    ms_help_bool(pp_key, pp_value,
                 gp_fs_env_trace_verbose, &gv_fs_trace_verbose);

    trace_set_assert_no_trace(gv_fs_assert_error);
    trace_set_delta(gv_fs_trace_file_delta);
    if (gv_fs_trace_file_nolock)
        trace_set_lock(!gv_fs_trace_file_nolock);
    // if now enabled, restart trace
    if ((!lv_old_enable) && gv_fs_trace_enable) {
        trace_init2(gp_fs_trace_file,
                    gv_fs_trace_file_unique,
                    gp_fs_trace_prefix,
                    false,
                    gv_fs_trace_file_maxsize);
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
    if (lv_old_enable && (!gv_fs_trace_enable))
        trace_flush();
    // if new fb, set
    if ((gv_fs_trace_file_fb > 0) && (lv_old_fb != gv_fs_trace_file_fb))
        trace_set_mem(gv_fs_trace_file_fb);
}

//
// possible scenarios:
// 1. key=MS_TRACE_TIMER
//    value=1
// 2. key=MS_TRACE_TIMER
//    value=1,MS_TRACE_MON=1[...]
//
void fs_trace_change_list(const char *pp_key, const char *pp_value) {
    char  la_value[1024];
    char *lp_key;
    char *lp_p;
    char *lp_value;

    lp_p = strchr(const_cast<char *>(pp_value), ',');
    if (lp_p == NULL)
        fs_trace_change_item(pp_key, pp_value);
    else {
        fs_trace_change_item(gp_fs_env_trace_enable, "0");
        SB_util_assert_stlt(strlen(pp_value), sizeof(la_value));
        strcpy(la_value, pp_value); // make a copy
        lp_p = strchr(la_value, ',');
        *lp_p = 0; // substitute eol
        fs_trace_change_item(pp_key, la_value); // deal with 1st iter
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
            fs_trace_change_item(lp_key, lp_value);
            if (lp_p == NULL)
                break;
            lp_key = &lp_p[1];
        }
        fs_trace_change_item(gp_fs_env_trace_enable, "1");
    }
}
