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

#include <signal.h>
#include <unistd.h>

#include "apictr.h"
#include "env.h"

#ifdef USE_SB_API_CTRS
SB_API::Ctr_Mgr gv_sb_api_ctr_mgr;

static const char *ga_sb_api_ctr_names[] = {
        "AFIRST",
        "BACTIVATERECEIVETRANSID",
        "BAWAITIOX",
        "BAWAITIOXTS",
        "BCANCEL",
        "BCANCELREQ",
        "BFILE_CLOSE_",
        "BFILE_COMPLETE_",
        "BFILE_COMPLETE_GETINFO_",
        "BFILE_COMPLETE_SET_",
        "BFILE_GETINFO_",
        "BFILE_GETRECEIVEINFO_",
        "BFILE_OPEN_",
        "BFILE_OPEN_SELF_",
        "BMSG_ABANDON_",
        "BMSG_AWAIT_",
        "BMSG_BREAK_",
        "BMSG_GETREQINFO_",
        "BMSG_HOLD_",
        "BMSG_ISCANCELED_",
        "BMSG_ISDONE_",
        "BMSG_LINK_",
        "BMSG_LISTEN_",
        "BMSG_READCTRL_",
        "BMSG_READDATA_",
        "BMSG_RELEASEALLHELD_",
        "BMSG_REPLY_",
        "BMSG_SETTAG_",
        "BREADUPDATEX",
        "BREADX",
        "BREPLYX",
        "BSETMODE",
        "BWRITEREADX",
        "BWRITEREADX2",
        "BWRITEX",
//      "FILE_BUF_OPTIONS",
        "file_buf_readupdatex",
//      "FILE_BUF_REGISTER",
//      "FILE_DEBUG_HOOK",
        "file_enable_open_cleanup",
//      "FILE_EVENT_DEREGISTER",
//      "FILE_EVENT_DISABLE_ABORT",
//      "FILE_EVENT_REGISTER",
        "file_init",
        "file_init_attach",
//      "FILE_MON_PROCESS_CLOSE",
//      "FILE_MON_PROCESS_SHUTDOWN",
//      "FILE_MON_PROCESS_SHUTDOWN_NOW",
//      "FILE_MON_PROCESS_STARTUP",
//      "FILE_TEST_ASSERT_DISABLE",
//      "FILE_TEST_ASSERT_ENABLE",
//      "FILE_TEST_INIT",
//      "FILE_TEST_PROFILER_FLUSH",
//      "FILE_TEST_PROFILER_MARK",
//      "FILE_TEST_PROFILER_RESET",
        "msg_buf_options",
        "msg_buf_read_ctrl",
        "msg_buf_read_data",
        "msg_buf_register",
        "msg_debug_hook",
        "msg_enable_lim_queue",
        "msg_enable_open_cleanup",
        "msg_enable_priority_queue",
        "msg_enable_recv_queue_proc_death",
        "msg_get_phandle",
        "msg_get_phandle_no_open",
        "msg_init",
        "msg_init_attach",
        "msg_init_trace",
        "msg_mon_close_process",
        "msg_mon_deregister_death_notification",
        "msg_mon_deregister_death_notification2",
        "msg_mon_deregister_death_notification3",
        "msg_mon_deregister_death_notification4",
        "msg_mon_deregister_death_notification_name",
        "msg_mon_deregister_death_notification_name2",
        "msg_mon_dump_process_id",
        "msg_mon_dump_process_name",
        "msg_mon_enable_mon_messages",
        "msg_mon_event_send",
        "msg_mon_event_send_name",
        "msg_mon_event_wait",
        "msg_mon_event_wait2",
        "msg_mon_get_monitor_stats",
        "msg_mon_get_my_info",
        "msg_mon_get_my_info2",
        "msg_mon_get_my_info3",
        "msg_mon_get_my_info4",
        "msg_mon_get_my_process_name",
        "msg_mon_get_my_segid",
        "msg_mon_get_node_info",
        "msg_mon_get_node_info2",
        "msg_mon_get_node_info_all",
        "msg_mon_get_node_info_detail",
        "msg_mon_get_open_info",
        "msg_mon_get_open_info_max",
        "msg_mon_get_process_info",
        "msg_mon_get_process_info2",
        "msg_mon_get_process_info_detail",
        "msg_mon_get_process_info_type",
        "msg_mon_get_process_name",
        "msg_mon_get_process_name2",
        "msg_mon_get_trans_info_process",
        "msg_mon_get_trans_info_transid",
        "msg_mon_get_zone_info",
        "msg_mon_get_zone_info_detail",
        "msg_mon_mount_device",
        "msg_mon_node_down",
        "msg_mon_node_down2",
        "msg_mon_node_up",
        "msg_mon_open_process",
        "msg_mon_open_process_backup",
        "msg_mon_open_process_fs",
        "msg_mon_open_process_ic",
        "msg_mon_open_process_nowait_cb",
        "msg_mon_open_process_self",
        "msg_mon_open_process_self_ic",
        "msg_mon_process_close",
        "msg_mon_process_shutdown",
        "msg_mon_process_shutdown_fast",
        "msg_mon_process_shutdown_now",
        "msg_mon_process_startup",
        "msg_mon_process_startup2",
        "msg_mon_process_startup3",
        "msg_mon_process_startup4",
        "msg_mon_reg_get",
        "msg_mon_reg_set",
        "msg_mon_register_death_notification",
        "msg_mon_register_death_notification2",
        "msg_mon_register_death_notification3",
        "msg_mon_register_death_notification4",
        "msg_mon_register_death_notification_name",
        "msg_mon_register_death_notification_name2",
        "msg_mon_reopen_process",
        "msg_mon_shutdown",
        "msg_mon_shutdown_now",
        "msg_mon_start_process",
        "msg_mon_start_process2",
        "msg_mon_start_process_nowait",
        "msg_mon_start_process_nowait2",
        "msg_mon_start_process_nowait_cb",
        "msg_mon_start_process_nowait_cb2",
        "msg_mon_stfsd_send",
        "msg_mon_stop_process",
        "msg_mon_stop_process2",
        "msg_mon_tm_leader_set",
        "msg_mon_tm_ready",
        "msg_mon_tmsync_issue",
        "msg_mon_tmsync_register",
        "msg_mon_trace_register_change",
        "msg_mon_trans_delist",
        "msg_mon_trans_end",
        "msg_mon_trans_enlist",
        "msg_mon_trans_register_tmlib",
        "msg_mon_trans_register_tmlib2",
        "msg_set_phandle",
//      "MSG_TEST_ASSERT_DISABLE",
//      "MSG_TEST_ASSERT_ENABLE",
//      "MSG_TEST_DISABLE_WAIT",
//      "MSG_TEST_ENABLE_CLIENT_ONLY",
//      "MSG_TEST_INIT",
//      "MSG_TEST_PROFILER_FLUSH",
//      "MSG_TEST_PROFILER_MARK",
//      "MSG_TEST_PROFILER_RESET",
//      "MSG_TEST_SET_MD_COUNT",
        "proc_enable_external_wakeups",
        "proc_event_deregister",
        "proc_event_disable_abort",
        "proc_event_register",
        "proc_register_group_pin",
        "proc_set_process_completion",
//      "SB_BACKTRACE",
//      "SB_BACKTRACE2",
//      "SB_LOG_ADD_ARRAY_TOKEN",
//      "SB_LOG_ADD_TOKEN",
//      "SB_LOG_ENABLE_LOGGING",
//      "SB_LOG_INIT",
//      "SB_LOG_INIT_COMPID",
//      "SB_LOG_TS_ADD_ARRAY_TOKEN",
//      "SB_LOG_TS_ADD_TOKEN",
//      "SB_LOG_TS_INIT",
//      "SB_LOG_TS_WRITE",
//      "SB_LOG_WRITE",
//      "SB_LOG_WRITE_STR",
//      "SB_MPI_TICK",
//      "SB_MPI_TIME",
        "sb_rpc_svcms_create",
//      "THREAD_RESUME_SUSPENDED",
//      "THREAD_SUSPEND_ALL",
        "timer_cancel",
        "timer_register",
        "timer_start_cb",
//      "XACTIVATERECEIVETRANSID",
//      "XAWAITIOX",
        "XAWAKE",
        "XAWAKE_A06",
//      "XCANCEL",
//      "XCANCELREQ",
        "XCANCELTIMEOUT",
        "XCONTROLMESSAGESYSTEM",
        "XDEBUG",
//      "XFILE_CLOSE_",
//      "XFILE_GETINFO_",
//      "XFILE_GETRECEIVEINFO_",
        "XFILENAME_TO_PROCESSHANDLE_",
//      "XFILE_OPEN_",
//      "XFILE_OPEN_SELF_",
        "XMESSAGESYSTEMINFO",
//      "XMSG_ABANDON_",
//      "XMSG_AWAIT_",
//      "XMSG_BREAK_",
//      "XMSG_GETREQINFO_",
//      "XMSG_HOLD_",
//      "XMSG_ISCANCELED_",
//      "XMSG_ISDONE_",
//      "XMSG_LINK_",
//      "XMSG_LISTEN_",
//      "XMSG_READCTRL_",
//      "XMSG_READDATA_",
//      "XMSG_RELEASEALLHELD_",
//      "XMSG_REPLY_",
//      "XMSG_SETTAG_",
        "XPROCESS_AWAKE_",
        "XPROCESS_GETPAIRINFO_",
        "XPROCESSHANDLE_COMPARE_",
        "XPROCESSHANDLE_DECOMPOSE_",
        "XPROCESSHANDLE_GETMINE_",
        "XPROCESSHANDLE_NULLIT_",
        "XPROCESSOR_GETINFOLIST_",
//      "XREADUPDATEX",
//      "XREADX",
//      "XREPLYX",
//      "XSETMODE",
        "XSIGNALTIMEOUT",
        "XWAIT",
        "XWAIT0",
        "XWAITNO0",
//      "XWRITEREADX",
//      "XWRITEREADX2",
//      "XWRITEX",
        "ZLAST"
};

static void sb_api_signal(int pv_sig, siginfo_t *pp_info, void *pp_ctx) {
    pv_sig = pv_sig; // touch
    pp_info = pp_info; // touch
    pp_ctx = pp_ctx; // touch
    gv_sb_api_ctr_mgr.report();
}

const char *SB_API::Ctr::get_name() {
    return  ga_sb_api_ctr_names[iv_ctr_type];
}

SB_API::Ctr_Mgr::Ctr_Mgr() : iv_ready(false), iv_reported(false) {
    char             *lp_p;
    struct sigaction  lv_act;
    int               lv_err;
    int               lv_sig;
    int               lv_stat;

    SB_util_static_assert(sizeof(ga_sb_api_ctr_names)/sizeof(char *) ==
                          (SB_ACTR_ZLAST + 1));
    gettimeofday(&iv_ts_start, NULL);
    for (lv_stat = SB_ACTR_AFIRST; lv_stat < SB_ACTR_ZLAST; lv_stat++) {
        ia_stats[lv_stat].iv_count = 0;
        ia_stats[lv_stat].iv_time_min = LLONG_MAX;
        ia_stats[lv_stat].iv_time_max = 0;
        ia_stats[lv_stat].iv_time_total = 0;
    }

    lv_sig = 0;
    lp_p = getenv(gp_ms_env_sb_api_sig);
    if (lp_p != NULL)
        lv_sig = atoi(lp_p);
    if (lv_sig) {
        lv_act.sa_sigaction = sb_api_signal;
        sigemptyset(&lv_act.sa_mask);
        sigaddset(&lv_act.sa_mask, SIGURG);
        lv_act.sa_flags = SA_SIGINFO;
        lv_err = sigaction(SIGURG, &lv_act, NULL);
        SB_util_assert_ieq(lv_err, 0);
    }
    iv_ready = true;
}

SB_API::Ctr_Mgr::~Ctr_Mgr() {
    report();
}

void SB_API::Ctr_Mgr::report() {
    char           la_cmdline[100];
    char           la_host[100];
    char           la_outfile[200];
    FILE          *lp_f;
    Stat_Type     *lp_stat;
    struct tm     *lp_time_tm;
    SB_Int64_Type  lv_count;
    pid_t          lv_pid;
    int            lv_stat;
    SB_Int64_Type  lv_time_avg;
    SB_Int64_Type  lv_time_elapsed;
    int            lv_time_ms;
    struct tm      lv_time_tm;
    SB_Int64_Type  lv_time_total;
    int            lv_time_us;

    if (iv_reported)
        return;
    iv_reported = true;
    gettimeofday(&iv_ts_stop, NULL);

    lv_count = 0;
    lv_time_total = 0;
    for (lv_stat = SB_ACTR_AFIRST + 1; lv_stat < SB_ACTR_ZLAST; lv_stat++) {
        lv_count += ia_stats[lv_stat].iv_count;
        lv_time_total += ia_stats[lv_stat].iv_time_total;
    }
    if (lv_count == 0)
        return;
    lv_time_total /= 1000; // ns -> us

    lp_f = fopen("/proc/self/cmdline", "r");
    SB_util_assert_pne(lp_f, NULL);
    fgets(la_cmdline, sizeof(la_cmdline), lp_f);
    fclose(lp_f);

    gethostname(la_host, sizeof(la_host));
    lv_pid = getpid();

    sprintf(la_outfile, "zsbapi.%s.%s.%d", la_host, la_cmdline, lv_pid);
    lp_f = fopen(la_outfile, "w");
    if (lp_f != NULL) {
        fprintf(lp_f, "SB API report host=%s, cmd=%s, pid=%d\n",
                la_host, la_cmdline, lv_pid);
        lp_time_tm = localtime_r(&iv_ts_start.tv_sec, &lv_time_tm);
        lv_time_ms = static_cast<int>(iv_ts_start.tv_usec / 1000);
        lv_time_us = static_cast<int>(iv_ts_start.tv_usec - lv_time_ms * 1000);
        fprintf(lp_f, "start time %02d:%02d:%02d.%03d.%03d\n",
                lp_time_tm->tm_hour, lp_time_tm->tm_min, lp_time_tm->tm_sec,
                lv_time_ms, lv_time_us);
        lp_time_tm = localtime_r(&iv_ts_stop.tv_sec, &lv_time_tm);
        lv_time_ms = static_cast<int>(iv_ts_stop.tv_usec / 1000);
        lv_time_us = static_cast<int>(iv_ts_stop.tv_usec - lv_time_ms * 1000);
        fprintf(lp_f, "stop time  %02d:%02d:%02d.%03d.%03d\n",
                lp_time_tm->tm_hour, lp_time_tm->tm_min, lp_time_tm->tm_sec,
                lv_time_ms, lv_time_us);
        lv_time_elapsed = iv_ts_stop.tv_sec * 1000000 -
                          iv_ts_start.tv_sec * 1000000 +
                          iv_ts_stop.tv_usec -
                          iv_ts_start.tv_usec;
        fprintf(lp_f, "elapsed time " PF64 " us\n", lv_time_elapsed);

        fprintf(lp_f, "%-40s%-13s%-15s%-13s%-17s%-15s\n",
                "api", "count", "min(us)", "max(us)", "total(us)", "avg(us)");
        for (lv_stat = SB_ACTR_AFIRST + 1; lv_stat < SB_ACTR_ZLAST; lv_stat++) {
            lp_stat = &ia_stats[lv_stat];
            if (lp_stat->iv_count > 0) {
                lp_stat->iv_time_min /= 1000; // ns -> us
                lp_stat->iv_time_max /= 1000; // ns -> us
                lp_stat->iv_time_total /= 1000; // ns -> us
                lv_time_avg = lp_stat->iv_time_total / lp_stat->iv_count;
#if __WORDSIZE == 64
                fprintf(lp_f,
                        "%-30s%15.1ld%15.1ld%15.1ld%15.1ld%15.1ld\n",
                        ga_sb_api_ctr_names[lv_stat],
                        lp_stat->iv_count,
                        lp_stat->iv_time_min,
                        lp_stat->iv_time_max,
                        lp_stat->iv_time_total,
                        lv_time_avg);
#else
                fprintf(lp_f,
                        "%-30s%15.1lld%15.1lld%15.1lld%15.1lld%15.1lld\n",
                        ga_sb_api_ctr_names[lv_stat],
                        lp_stat->iv_count,
                        lp_stat->iv_time_min,
                        lp_stat->iv_time_max,
                        lp_stat->iv_time_total,
                        lv_time_avg);
#endif
            }
        }
#if __WORDSIZE == 64
        fprintf(lp_f,
                "%-30s%15.1ld%45.1ld\n",
                "<total>",
                lv_count,
                lv_time_total);
#else
        fprintf(lp_f,
                "%-30s%15.1lld%45.1lld\n",
                "<total>",
                lv_count,
                lv_time_total);
#endif

        fprintf(lp_f, "\n");
        fclose(lp_f);
    }
}
#endif

