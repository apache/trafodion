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

#ifndef __SB_MSTRACE_H_
#define __SB_MSTRACE_H_

// globals
extern bool        gv_ms_trace;
extern bool        gv_ms_trace_abandon;
extern bool        gv_ms_trace_alloc;
extern bool        gv_ms_trace_data;
extern int         gv_ms_trace_data_max;
extern bool        gv_ms_trace_detail;
extern bool        gv_ms_trace_dialect;
extern bool        gv_ms_trace_enable;
extern bool        gv_ms_trace_environ;
extern bool        gv_ms_trace_errors;
extern bool        gv_ms_trace_events;
extern bool        gv_ms_trace_evlog;
extern bool        gv_ms_trace_file_delta;
extern int         gv_ms_trace_file_fb;
extern int         gv_ms_trace_file_inmem;
extern long long   gv_ms_trace_file_maxsize;
extern bool        gv_ms_trace_file_nolock;
extern bool        gv_ms_trace_file_signal;
extern int         gv_ms_trace_file_unique;
extern bool        gv_ms_trace_ic;
extern bool        gv_ms_trace_locio;
extern bool        gv_ms_trace_md;
extern bool        gv_ms_trace_mon;
extern bool        gv_ms_trace_name;
extern bool        gv_ms_trace_params;
extern bool        gv_ms_trace_qalloc;
extern bool        gv_ms_trace_ref;
extern bool        gv_ms_trace_sm;
extern bool        gv_ms_trace_sock;
extern bool        gv_ms_trace_stats;
extern bool        gv_ms_trace_timer;
extern bool        gv_ms_trace_timermap;
extern bool        gv_ms_trace_trans;
extern bool        gv_ms_trace_verbose;
extern bool        gv_ms_trace_wait;
extern int         gv_ms_trace_xx;
extern bool        gv_sb_trace_pthread;
extern bool        gv_sb_trace_thread;

enum { MS_TRACE_XX_NIDPID = 1 };

extern void        ms_help_bool(const char *pp_key, const char *pp_value,
                                const char *pp_key_cmp, bool *pp_value_ret);

extern void        ms_help_int(const char *pp_key, const char *pp_value,
                               const char *pp_key_cmp, int *pp_value_ret);
extern void        ms_help_longlong(const char *pp_key,
                                    const char *pp_value,
                                    const char *pp_key_cmp,
                                    long long  *pp_value_ret);
extern const char *ms_help_str(const char *pp_key, const char *pp_value,
                               const char *pp_key_cmp);
// "C" for dlsym
extern "C" void    ms_trace_change_list(const char *pp_key,
                                        const char *pp_value);
extern void        ms_trace_change_item(const char *pp_key,
                                        const char *pp_value);
extern void        ms_trace_signal_init();

#endif // !__SB_MSTRACE_H_
