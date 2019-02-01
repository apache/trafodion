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

#ifndef __SB_ENV_H_
#define __SB_ENV_H_

// env string externs
extern const char *gp_fs_env_assert_error;
extern const char *gp_fs_env_max_cap_fds;

extern const char *gp_fs_env_trace;
extern const char *gp_fs_env_trace_data;
extern const char *gp_fs_env_trace_data_max;
extern const char *gp_fs_env_trace_detail;
extern const char *gp_fs_env_trace_enable;
extern const char *gp_fs_env_trace_errors;
extern const char *gp_fs_env_trace_file;
extern const char *gp_fs_env_trace_file_delta;
extern const char *gp_fs_env_trace_file_dir;
extern const char *gp_fs_env_trace_file_fb;
extern const char *gp_fs_env_trace_file_maxsize;
extern const char *gp_fs_env_trace_file_nolock;
extern const char *gp_fs_env_trace_file_unique;
extern const char *gp_fs_env_trace_mon;
extern const char *gp_fs_env_trace_mt;
extern const char *gp_fs_env_trace_params;
extern const char *gp_fs_env_trace_params0;
extern const char *gp_fs_env_trace_prefix;
extern const char *gp_fs_env_trace_verbose;

extern const char *gp_ms_env_assert_chk;
extern const char *gp_ms_env_assert_chk_send;
extern const char *gp_ms_env_assert_error;
extern const char *gp_ms_env_conn_idle_timeout;
extern const char *gp_ms_env_conn_reuse;
extern const char *gp_ms_env_debug;
extern const char *gp_ms_env_disc_sem;
extern const char *gp_ms_env_disc_sem_rob;
extern const char *gp_ms_env_disc_sem_stats;
extern const char *gp_ms_env_hook_enable;
extern const char *gp_ms_env_max_cap_mds;
extern const char *gp_ms_env_max_cap_ods;
extern const char *gp_ms_env_max_cap_phandles;
extern const char *gp_ms_env_mpi_tmpdir;
extern const char *gp_ms_env_msg_timestamp;
extern const char *gp_ms_env_sb_api_sig;
extern const char *gp_ms_env_shutdown_fast;
extern const char *gp_ms_env_sq_ic;
extern const char *gp_ms_env_sq_msgflow;
extern const char *gp_ms_env_sq_snapshot_dir;
extern const char *gp_ms_env_sq_root;
extern const char *gp_ms_env_sq_conf;
extern const char *gp_ms_env_sq_log;
extern const char *gp_ms_env_sq_var;
extern const char *gp_ms_env_sq_trans_mpi;
extern const char *gp_ms_env_sq_trans_sm;
extern const char *gp_ms_env_sq_trans_sock;
extern const char *gp_ms_env_sq_vnid;
extern const char *gp_ms_env_sq_vnodes;
extern const char *gp_ms_env_streams_max;
extern const char *gp_ms_env_streams_min;

extern const char *gp_ms_env_trace;
extern const char *gp_ms_env_trace_abandon;
extern const char *gp_ms_env_trace_alloc;
extern const char *gp_ms_env_trace_data;
extern const char *gp_ms_env_trace_data_max;
extern const char *gp_ms_env_trace_detail;
extern const char *gp_ms_env_trace_dialect;
extern const char *gp_ms_env_trace_enable;
extern const char *gp_ms_env_trace_environ;
extern const char *gp_ms_env_trace_errors;
extern const char *gp_ms_env_trace_events;
extern const char *gp_ms_env_trace_evlog;
extern const char *gp_ms_env_trace_file;
extern const char *gp_ms_env_trace_file_delta;
extern const char *gp_ms_env_trace_file_dir;
extern const char *gp_ms_env_trace_file_fb;
extern const char *gp_ms_env_trace_file_inmem;
extern const char *gp_ms_env_trace_file_maxsize;
extern const char *gp_ms_env_trace_file_nolock;
extern const char *gp_ms_env_trace_file_signal;
extern const char *gp_ms_env_trace_file_unique;
extern const char *gp_ms_env_trace_ic;
extern const char *gp_ms_env_trace_locio;
extern const char *gp_ms_env_trace_md;
extern const char *gp_ms_env_trace_mon;
extern const char *gp_ms_env_trace_name;
extern const char *gp_ms_env_trace_params;
extern const char *gp_ms_env_trace_pthread;
extern const char *gp_ms_env_trace_prefix;
extern const char *gp_ms_env_trace_qalloc;
extern const char *gp_ms_env_trace_ref;
extern const char *gp_ms_env_trace_rpc;
extern const char *gp_ms_env_trace_rpc_data;
extern const char *gp_ms_env_trace_rpc_data_max;
extern const char *gp_ms_env_trace_rpc_msg;
extern const char *gp_ms_env_trace_sm;
extern const char *gp_ms_env_trace_smt;
extern const char *gp_ms_env_trace_smt_data;
extern const char *gp_ms_env_trace_sock;
extern const char *gp_ms_env_trace_stats;
extern const char *gp_ms_env_trace_thread;
extern const char *gp_ms_env_trace_timer;
extern const char *gp_ms_env_trace_timermap;
extern const char *gp_ms_env_trace_trans;
extern const char *gp_ms_env_trace_verbose;
extern const char *gp_ms_env_trace_wait;
extern const char *gp_ms_env_trace_xx;

#endif // !__SB_ENV_H_
