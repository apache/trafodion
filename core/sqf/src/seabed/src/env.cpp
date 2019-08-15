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

// FS keywords
const char *gp_fs_env_assert_error       = "FS_ASSERT_ERROR";
const char *gp_fs_env_max_cap_fds        = "FS_MAX_CAP_FDS";

const char *gp_fs_env_trace              = "FS_TRACE";
const char *gp_fs_env_trace_data         = "FS_TRACE_DATA";
const char *gp_fs_env_trace_data_max     = "FS_TRACE_DATA_MAX";
const char *gp_fs_env_trace_detail       = "FS_TRACE_DETAIL";
const char *gp_fs_env_trace_enable       = "FS_TRACE_ENABLE";
const char *gp_fs_env_trace_errors       = "FS_TRACE_ERRORS";
const char *gp_fs_env_trace_file         = "FS_TRACE_FILE";
const char *gp_fs_env_trace_file_delta   = "FS_TRACE_FILE_DELTA";
const char *gp_fs_env_trace_file_dir     = "FS_TRACE_FILE_DIR";
const char *gp_fs_env_trace_file_fb      = "FS_TRACE_FILE_FB";
const char *gp_fs_env_trace_file_maxsize = "FS_TRACE_FILE_MAXSIZE";
const char *gp_fs_env_trace_file_nolock  = "FS_TRACE_FILE_NOLOCK";
const char *gp_fs_env_trace_file_unique  = "FS_TRACE_FILE_UNIQUE";
const char *gp_fs_env_trace_mon          = "FS_TRACE_MON";
const char *gp_fs_env_trace_mt           = "FS_TRACE_MT";
const char *gp_fs_env_trace_params       = "FS_TRACE_PARAMS";
const char *gp_fs_env_trace_params0      = "FS_TRACE_PARAMS0";
const char *gp_fs_env_trace_prefix       = "FS_TRACE_PREFIX";
const char *gp_fs_env_trace_sm           = "FS_TRACE_SM";  // statemachine
const char *gp_fs_env_trace_verbose      = "FS_TRACE_VERBOSE";

// MS keywords
const char *gp_ms_env_assert_chk         = "MS_ASSERT_CHK";
const char *gp_ms_env_assert_chk_send    = "MS_ASSERT_CHK_SEND";
const char *gp_ms_env_assert_error       = "MS_ASSERT_ERROR";
const char *gp_ms_env_conn_idle_timeout  = "MS_CONN_IDLE_TIMEOUT";
const char *gp_ms_env_conn_reuse         = "MS_CONN_REUSE";
const char *gp_ms_env_debug              = "MS_DEBUG";
const char *gp_ms_env_disc_sem           = "MS_DISC_SEM";
const char *gp_ms_env_disc_sem_rob       = "MS_DISC_SEM_ROBUST";
const char *gp_ms_env_disc_sem_stats     = "MS_DISC_SEM_STATS";
const char *gp_ms_env_hook_enable        = "HOOK_ENABLE";
const char *gp_ms_env_max_cap_mds        = "MS_MAX_CAP_MDS";
const char *gp_ms_env_max_cap_ods        = "MS_MAX_CAP_ODS";
const char *gp_ms_env_max_cap_phandles   = "MS_MAX_CAP_PHANDLES";
const char *gp_ms_env_mpi_tmpdir         = "TRAF_LOG";
const char *gp_ms_env_msg_timestamp      = "MS_MSG_TIMESTAMP";
const char *gp_ms_env_sb_api_sig         = "SB_API_SIG";
const char *gp_ms_env_shutdown_fast      = "MS_SHUTDOWN_FAST";
const char *gp_ms_env_sq_ic              = "SQ_IC";
const char *gp_ms_env_sq_msgflow         = "SQ_MSGFLOW";
const char *gp_ms_env_sq_snapshot_dir    = "SQ_SNAPSHOT_DIR";
const char *gp_ms_env_sq_trans_sock      = "SQ_TRANS_SOCK";
const char *gp_ms_env_sq_root            = "TRAF_HOME";
const char *gp_ms_env_sq_conf            = "TRAF_CONF";
const char *gp_ms_env_sq_log             = "TRAF_LOG";
const char *gp_ms_env_sq_var             = "TRAF_VAR";
const char *gp_ms_env_sq_vnid            = "SQ_VIRTUAL_NID";
const char *gp_ms_env_sq_vnodes          = "SQ_VIRTUAL_NODES";
const char *gp_ms_env_streams_max        = "MS_STREAMS_MAX";
const char *gp_ms_env_streams_min        = "MS_STREAMS_MIN";

const char *gp_ms_env_trace              = "MS_TRACE";
const char *gp_ms_env_trace_abandon      = "MS_TRACE_ABANDON";
const char *gp_ms_env_trace_alloc        = "MS_TRACE_ALLOC";
const char *gp_ms_env_trace_data         = "MS_TRACE_DATA";
const char *gp_ms_env_trace_data_max     = "MS_TRACE_DATA_MAX";
const char *gp_ms_env_trace_detail       = "MS_TRACE_DETAIL";
const char *gp_ms_env_trace_dialect      = "MS_TRACE_DIALECT";
const char *gp_ms_env_trace_enable       = "MS_TRACE_ENABLE";
const char *gp_ms_env_trace_environ      = "MS_TRACE_ENVIRON";
const char *gp_ms_env_trace_errors       = "MS_TRACE_ERRORS";
const char *gp_ms_env_trace_events       = "MS_TRACE_EVENTS";
const char *gp_ms_env_trace_evlog        = "MS_TRACE_EVLOG";
const char *gp_ms_env_trace_file         = "MS_TRACE_FILE";
const char *gp_ms_env_trace_file_delta   = "MS_TRACE_FILE_DELTA";
const char *gp_ms_env_trace_file_dir     = "MS_TRACE_FILE_DIR";
const char *gp_ms_env_trace_file_fb      = "MS_TRACE_FILE_FB";
const char *gp_ms_env_trace_file_inmem   = "MS_TRACE_FILE_INMEM";
const char *gp_ms_env_trace_file_maxsize = "MS_TRACE_FILE_MAXSIZE";
const char *gp_ms_env_trace_file_nolock  = "MS_TRACE_FILE_NOLOCK";
const char *gp_ms_env_trace_file_signal  = "MS_TRACE_FILE_SIGNAL";
const char *gp_ms_env_trace_file_unique  = "MS_TRACE_FILE_UNIQUE";
const char *gp_ms_env_trace_ic           = "MS_TRACE_IC";
const char *gp_ms_env_trace_locio        = "MS_TRACE_LOCIO";
const char *gp_ms_env_trace_md           = "MS_TRACE_MD";
const char *gp_ms_env_trace_mon          = "MS_TRACE_MON";
const char *gp_ms_env_trace_name         = "MS_TRACE_NAME";
const char *gp_ms_env_trace_params       = "MS_TRACE_PARAMS";
const char *gp_ms_env_trace_pthread      = "MS_TRACE_PTHREAD";
const char *gp_ms_env_trace_prefix       = "MS_TRACE_PREFIX";
const char *gp_ms_env_trace_qalloc       = "MS_TRACE_QALLOC";
const char *gp_ms_env_trace_ref          = "MS_TRACE_REF";
const char *gp_ms_env_trace_sm           = "MS_TRACE_SM";  // statemachine
const char *gp_ms_env_trace_sock         = "MS_TRACE_SOCK";
const char *gp_ms_env_trace_stats        = "MS_TRACE_STATS";
const char *gp_ms_env_trace_thread       = "MS_TRACE_THREAD";
const char *gp_ms_env_trace_timer        = "MS_TRACE_TIMER";
const char *gp_ms_env_trace_timermap     = "MS_TRACE_TIMERMAP";
const char *gp_ms_env_trace_trans        = "MS_TRACE_TRANS";
const char *gp_ms_env_trace_verbose      = "MS_TRACE_VERBOSE";
const char *gp_ms_env_trace_wait         = "MS_TRACE_WAIT";
const char *gp_ms_env_trace_xx           = "MS_TRACE_XX";
