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

#include <limits.h>
#include <stdio.h>

// globals
char           *gp_ms_trace_file           = NULL;
char           *gp_ms_trace_file_dir       = NULL;
char           *gp_ms_trace_prefix         = const_cast<char *>("MS");
bool            gv_ms_trace                = false;
bool            gv_ms_trace_abandon        = false;
bool            gv_ms_trace_alloc          = false;
bool            gv_ms_trace_data           = false;
int             gv_ms_trace_data_max       = -1;
bool            gv_ms_trace_detail         = false;
bool            gv_ms_trace_dialect        = false;
bool            gv_ms_trace_enable         = false;
bool            gv_ms_trace_environ        = false;
bool            gv_ms_trace_errors         = false;
bool            gv_ms_trace_events         = false;
bool            gv_ms_trace_evlog          = false;
bool            gv_ms_trace_file_delta     = false;
int             gv_ms_trace_file_fb        = 0;
int             gv_ms_trace_file_inmem     = 0;
long long       gv_ms_trace_file_maxsize   = INT_MAX;
bool            gv_ms_trace_file_nolock    = false;
bool            gv_ms_trace_file_signal    = false;
int             gv_ms_trace_file_unique    = 0;
bool            gv_ms_trace_ic             = false;
bool            gv_ms_trace_locio          = false;
bool            gv_ms_trace_md             = false;
bool            gv_ms_trace_mon            = false;
bool            gv_ms_trace_name           = false;
bool            gv_ms_trace_params         = false;
bool            gv_ms_trace_qalloc         = false;
bool            gv_ms_trace_ref            = false;
bool            gv_ms_trace_sm             = false;
bool            gv_ms_trace_sock           = false;
bool            gv_ms_trace_stats          = false;
bool            gv_ms_trace_timer          = false;
bool            gv_ms_trace_timermap       = false;
bool            gv_ms_trace_trans          = false;
bool            gv_ms_trace_verbose        = false;
bool            gv_ms_trace_wait           = false;
int             gv_ms_trace_xx             = 0;
bool            gv_sb_trace_pthread        = false;
bool            gv_sb_trace_thread         = false;

