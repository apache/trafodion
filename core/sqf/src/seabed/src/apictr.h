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

#ifndef __SB_APICTR_H_
#define __SB_APICTR_H_

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/time.h>

#include "seabed/int/opts.h"

#include "seabed/thread.h"


namespace SB_API {
#ifdef USE_SB_API_CTRS
    //
    // this list contains every API marked 'SB_Export' from
    // export/include/seabed
    //
    // commented out names are either duplicates or not implemented
    // an example duplicate would be FILE_BUF_OPTIONS as
    // file_buf_options simply calls msg_buf_options
    // an example not-implemented is MSG_TEST_ASSERT_DISABLE
    //
    typedef enum Ctr_Type {
        SB_ACTR_AFIRST,
        SB_ACTR_BACTIVATERECEIVETRANSID,
        SB_ACTR_BAWAITIOX,
        SB_ACTR_BAWAITIOXTS,
        SB_ACTR_BCANCEL,
        SB_ACTR_BCANCELREQ,
        SB_ACTR_BFILE_CLOSE_,
        SB_ACTR_BFILE_COMPLETE_,
        SB_ACTR_BFILE_COMPLETE_GETINFO_,
        SB_ACTR_BFILE_COMPLETE_SET_,
        SB_ACTR_BFILE_GETINFO_,
        SB_ACTR_BFILE_GETRECEIVEINFO_,
        SB_ACTR_BFILE_OPEN_,
        SB_ACTR_BFILE_OPEN_SELF_,
        SB_ACTR_BMSG_ABANDON_,
        SB_ACTR_BMSG_AWAIT_,
        SB_ACTR_BMSG_BREAK_,
        SB_ACTR_BMSG_GETREQINFO_,
        SB_ACTR_BMSG_HOLD_,
        SB_ACTR_BMSG_ISCANCELED_,
        SB_ACTR_BMSG_ISDONE_,
        SB_ACTR_BMSG_LINK_,
        SB_ACTR_BMSG_LISTEN_,
        SB_ACTR_BMSG_READCTRL_,
        SB_ACTR_BMSG_READDATA_,
        SB_ACTR_BMSG_RELEASEALLHELD_,
        SB_ACTR_BMSG_REPLY_,
        SB_ACTR_BMSG_SETTAG_,
        SB_ACTR_BREADUPDATEX,
        SB_ACTR_BREADX,
        SB_ACTR_BREPLYX,
        SB_ACTR_BSETMODE,
        SB_ACTR_BWRITEREADX,
        SB_ACTR_BWRITEREADX2,
        SB_ACTR_BWRITEX,
//      SB_ACTR_FILE_BUF_OPTIONS,
        SB_ACTR_FILE_BUF_READUPDATEX,
//      SB_ACTR_FILE_BUF_REGISTER,
//      SB_ACTR_FILE_DEBUG_HOOK,
        SB_ACTR_FILE_ENABLE_OPEN_CLEANUP,
//      SB_ACTR_FILE_EVENT_DEREGISTER,
//      SB_ACTR_FILE_EVENT_DISABLE_ABORT,
//      SB_ACTR_FILE_EVENT_REGISTER,
        SB_ACTR_FILE_INIT,
        SB_ACTR_FILE_INIT_ATTACH,
//      SB_ACTR_FILE_MON_PROCESS_CLOSE,
//      SB_ACTR_FILE_MON_PROCESS_SHUTDOWN,
//      SB_ACTR_FILE_MON_PROCESS_SHUTDOWN_NOW,
//      SB_ACTR_FILE_MON_PROCESS_STARTUP,
//      SB_ACTR_FILE_TEST_ASSERT_DISABLE,
//      SB_ACTR_FILE_TEST_ASSERT_ENABLE,
//      SB_ACTR_FILE_TEST_INIT,
//      SB_ACTR_FILE_TEST_PROFILER_FLUSH,
//      SB_ACTR_FILE_TEST_PROFILER_MARK,
//      SB_ACTR_FILE_TEST_PROFILER_RESET,
        SB_ACTR_MSG_BUF_OPTIONS,
        SB_ACTR_MSG_BUF_READ_CTRL,
        SB_ACTR_MSG_BUF_READ_DATA,
        SB_ACTR_MSG_BUF_REGISTER,
        SB_ACTR_MSG_DEBUG_HOOK,
        SB_ACTR_MSG_ENABLE_LIM_QUEUE,
        SB_ACTR_MSG_ENABLE_OPEN_CLEANUP,
        SB_ACTR_MSG_ENABLE_PRIORITY_QUEUE,
        SB_ACTR_MSG_ENABLE_RECV_QUEUE_PROC_DEATH,
        SB_ACTR_MSG_GET_PHANDLE,
        SB_ACTR_MSG_GET_PHANDLE_NO_OPEN,
        SB_ACTR_MSG_INIT,
        SB_ACTR_MSG_INIT_ATTACH,
        SB_ACTR_MSG_INIT_TRACE,
        SB_ACTR_MSG_MON_CLOSE_PROCESS,
        SB_ACTR_MSG_MON_DEREGISTER_DEATH_NOTIFICATION,
        SB_ACTR_MSG_MON_DEREGISTER_DEATH_NOTIFICATION2,
        SB_ACTR_MSG_MON_DEREGISTER_DEATH_NOTIFICATION3,
        SB_ACTR_MSG_MON_DEREGISTER_DEATH_NOTIFICATION4,
        SB_ACTR_MSG_MON_DEREGISTER_DEATH_NOTIFICATION_NAME,
        SB_ACTR_MSG_MON_DEREGISTER_DEATH_NOTIFICATION_NAME2,
        SB_ACTR_MSG_MON_DUMP_PROCESS_ID,
        SB_ACTR_MSG_MON_DUMP_PROCESS_NAME,
        SB_ACTR_MSG_MON_ENABLE_MON_MESSAGES,
        SB_ACTR_MSG_MON_EVENT_SEND,
        SB_ACTR_MSG_MON_EVENT_SEND_NAME,
        SB_ACTR_MSG_MON_EVENT_WAIT,
        SB_ACTR_MSG_MON_EVENT_WAIT2,
        SB_ACTR_MSG_MON_GET_MONITOR_STATS,
        SB_ACTR_MSG_MON_GET_MY_INFO,
        SB_ACTR_MSG_MON_GET_MY_INFO2,
        SB_ACTR_MSG_MON_GET_MY_INFO3,
        SB_ACTR_MSG_MON_GET_MY_INFO4,
        SB_ACTR_MSG_MON_GET_MY_PROCESS_NAME,
        SB_ACTR_MSG_MON_GET_MY_SEGID,
        SB_ACTR_MSG_MON_GET_NODE_INFO,
        SB_ACTR_MSG_MON_GET_NODE_INFO2,
        SB_ACTR_MSG_MON_GET_NODE_INFO_ALL,
        SB_ACTR_MSG_MON_GET_NODE_INFO_DETAIL,
        SB_ACTR_MSG_MON_GET_OPEN_INFO,
        SB_ACTR_MSG_MON_GET_OPEN_INFO_MAX,
        SB_ACTR_MSG_MON_GET_PROCESS_INFO,
        SB_ACTR_MSG_MON_GET_PROCESS_INFO2,
        SB_ACTR_MSG_MON_GET_PROCESS_INFO_DETAIL,
        SB_ACTR_MSG_MON_GET_PROCESS_INFO_TYPE,
        SB_ACTR_MSG_MON_GET_PROCESS_NAME,
        SB_ACTR_MSG_MON_GET_PROCESS_NAME2,
        SB_ACTR_MSG_MON_GET_TM_SEQ,
        SB_ACTR_MSG_MON_GET_TRANS_INFO_PROCESS,
        SB_ACTR_MSG_MON_GET_TRANS_INFO_TRANSID,
        SB_ACTR_MSG_MON_GET_ZONE_INFO,
        SB_ACTR_MSG_MON_GET_ZONE_INFO_DETAIL,
        SB_ACTR_MSG_MON_GET_INSTANCE_ID,
        SB_ACTR_MSG_MON_MOUNT_DEVICE,
        SB_ACTR_MSG_MON_NODE_DOWN,
        SB_ACTR_MSG_MON_NODE_DOWN2,
        SB_ACTR_MSG_MON_NODE_UP,
        SB_ACTR_MSG_MON_OPEN_PROCESS,
        SB_ACTR_MSG_MON_OPEN_PROCESS_BACKUP,
        SB_ACTR_MSG_MON_OPEN_PROCESS_FS,
        SB_ACTR_MSG_MON_OPEN_PROCESS_IC,
        SB_ACTR_MSG_MON_OPEN_PROCESS_NOWAIT_CB,
        SB_ACTR_MSG_MON_OPEN_PROCESS_SELF,
        SB_ACTR_MSG_MON_OPEN_PROCESS_SELF_IC,
        SB_ACTR_MSG_MON_PROCESS_CLOSE,
        SB_ACTR_MSG_MON_PROCESS_SHUTDOWN,
        SB_ACTR_MSG_MON_PROCESS_SHUTDOWN_FAST,
        SB_ACTR_MSG_MON_PROCESS_SHUTDOWN_NOW,
        SB_ACTR_MSG_MON_PROCESS_STARTUP,
        SB_ACTR_MSG_MON_PROCESS_STARTUP2,
        SB_ACTR_MSG_MON_PROCESS_STARTUP3,
        SB_ACTR_MSG_MON_PROCESS_STARTUP4,
        SB_ACTR_MSG_MON_REG_GET,
        SB_ACTR_MSG_MON_REG_SET,
        SB_ACTR_MSG_MON_REGISTER_DEATH_NOTIFICATION,
        SB_ACTR_MSG_MON_REGISTER_DEATH_NOTIFICATION2,
        SB_ACTR_MSG_MON_REGISTER_DEATH_NOTIFICATION3,
        SB_ACTR_MSG_MON_REGISTER_DEATH_NOTIFICATION4,
        SB_ACTR_MSG_MON_REGISTER_DEATH_NOTIFICATION_NAME,
        SB_ACTR_MSG_MON_REGISTER_DEATH_NOTIFICATION_NAME2,
        SB_ACTR_MSG_MON_REOPEN_PROCESS,
        SB_ACTR_MSG_MON_SHUTDOWN,
        SB_ACTR_MSG_MON_SHUTDOWN_NOW,
        SB_ACTR_MSG_MON_START_PROCESS,
        SB_ACTR_MSG_MON_START_PROCESS2,
        SB_ACTR_MSG_MON_START_PROCESS_NOWAIT,
        SB_ACTR_MSG_MON_START_PROCESS_NOWAIT2,
        SB_ACTR_MSG_MON_START_PROCESS_NOWAIT_CB,
        SB_ACTR_MSG_MON_START_PROCESS_NOWAIT_CB2,
        SB_ACTR_MSG_MON_STFSD_SEND,
        SB_ACTR_MSG_MON_STOP_PROCESS,
        SB_ACTR_MSG_MON_STOP_PROCESS2,
        SB_ACTR_MSG_MON_TM_LEADER_SET,
        SB_ACTR_MSG_MON_TM_READY,
        SB_ACTR_MSG_MON_TMSYNC_ISSUE,
        SB_ACTR_MSG_MON_TMSYNC_REGISTER,
        SB_ACTR_MSG_MON_TRACE_REGISTER_CHANGE,
        SB_ACTR_MSG_MON_TRANS_DELIST,
        SB_ACTR_MSG_MON_TRANS_END,
        SB_ACTR_MSG_MON_TRANS_ENLIST,
        SB_ACTR_MSG_MON_TRANS_REGISTER_TMLIB,
        SB_ACTR_MSG_MON_TRANS_REGISTER_TMLIB2,
        SB_ACTR_MSG_SET_PHANDLE,
//      SB_ACTR_MSG_TEST_ASSERT_DISABLE,
//      SB_ACTR_MSG_TEST_ASSERT_ENABLE,
//      SB_ACTR_MSG_TEST_DISABLE_WAIT,
//      SB_ACTR_MSG_TEST_ENABLE_CLIENT_ONLY,
//      SB_ACTR_MSG_TEST_INIT,
//      SB_ACTR_MSG_TEST_PROFILER_FLUSH,
//      SB_ACTR_MSG_TEST_PROFILER_MARK,
//      SB_ACTR_MSG_TEST_PROFILER_RESET,
//      SB_ACTR_MSG_TEST_SET_MD_COUNT,
        SB_ACTR_PROC_ENABLE_EXTERNAL_WAKEUPS,
        SB_ACTR_PROC_EVENT_DEREGISTER,
        SB_ACTR_PROC_EVENT_DISABLE_ABORT,
        SB_ACTR_PROC_EVENT_REGISTER,
        SB_ACTR_PROC_REGISTER_GROUP_PIN,
        SB_ACTR_PROC_SET_PROCESS_COMPLETION,
//      SB_ACTR_SB_BACKTRACE,
//      SB_ACTR_SB_BACKTRACE2,
//      SB_ACTR_SB_LOG_ADD_ARRAY_TOKEN,
//      SB_ACTR_SB_LOG_ADD_TOKEN,
//      SB_ACTR_SB_LOG_ENABLE_LOGGING,
//      SB_ACTR_SB_LOG_INIT,
//      SB_ACTR_SB_LOG_INIT_COMPID,
//      SB_ACTR_SB_LOG_TS_ADD_ARRAY_TOKEN,
//      SB_ACTR_SB_LOG_TS_ADD_TOKEN,
//      SB_ACTR_SB_LOG_TS_INIT,
//      SB_ACTR_SB_LOG_TS_WRITE,
//      SB_ACTR_SB_LOG_WRITE,
//      SB_ACTR_SB_LOG_WRITE_STR,
//      SB_ACTR_SB_MPI_TICK,
//      SB_ACTR_SB_MPI_TIME,
        SB_ACTR_SB_RPC_SVCMS_CREATE,
//      SB_ACTR_THREAD_RESUME_SUSPENDED,
//      SB_ACTR_THREAD_SUSPEND_ALL,
        SB_ACTR_TIMER_CANCEL,
        SB_ACTR_TIMER_REGISTER,
        SB_ACTR_TIMER_START_CB,
//      SB_ACTR_XACTIVATERECEIVETRANSID,
//      SB_ACTR_XAWAITIOX,
        SB_ACTR_XAWAKE,
        SB_ACTR_XAWAKE_A06,
//      SB_ACTR_XCANCEL,
//      SB_ACTR_XCANCELREQ,
        SB_ACTR_XCANCELTIMEOUT,
        SB_ACTR_XCONTROLMESSAGESYSTEM,
        SB_ACTR_XDEBUG,
//      SB_ACTR_XFILE_CLOSE_,
//      SB_ACTR_XFILE_GETINFO_,
//      SB_ACTR_XFILE_GETRECEIVEINFO_,
        SB_ACTR_XFILENAME_TO_PROCESSHANDLE_,
//      SB_ACTR_XFILE_OPEN_,
//      SB_ACTR_XFILE_OPEN_SELF_,
        SB_ACTR_XMESSAGESYSTEMINFO,
//      SB_ACTR_XMSG_ABANDON_,
//      SB_ACTR_XMSG_AWAIT_,
//      SB_ACTR_XMSG_BREAK_,
//      SB_ACTR_XMSG_GETREQINFO_,
//      SB_ACTR_XMSG_HOLD_,
//      SB_ACTR_XMSG_ISCANCELED_,
//      SB_ACTR_XMSG_ISDONE_,
//      SB_ACTR_XMSG_LINK_,
//      SB_ACTR_XMSG_LISTEN_,
//      SB_ACTR_XMSG_READCTRL_,
//      SB_ACTR_XMSG_READDATA_,
//      SB_ACTR_XMSG_RELEASEALLHELD_,
//      SB_ACTR_XMSG_REPLY_,
//      SB_ACTR_XMSG_SETTAG_,
        SB_ACTR_XPROCESS_AWAKE_,
        SB_ACTR_XPROCESS_GETPAIRINFO_,
        SB_ACTR_XPROCESSHANDLE_COMPARE_,
        SB_ACTR_XPROCESSHANDLE_DECOMPOSE_,
        SB_ACTR_XPROCESSHANDLE_GETMINE_,
        SB_ACTR_XPROCESSHANDLE_NULLIT_,
        SB_ACTR_XPROCESSOR_GETINFOLIST_,
//      SB_ACTR_XREADUPDATEX,
//      SB_ACTR_XREADX,
//      SB_ACTR_XREPLYX,
//      SB_ACTR_XSETMODE,
        SB_ACTR_XSIGNALTIMEOUT,
        SB_ACTR_XWAIT,
        SB_ACTR_XWAIT0,
        SB_ACTR_XWAITNO0,
//      SB_ACTR_XWRITEREADX,
//      SB_ACTR_XWRITEREADX2,
//      SB_ACTR_XWRITEX,
        SB_ACTR_ZLAST
    } Ctr_Type;

    class Ctr_Mgr;

    class Ctr {
    public:
        Ctr(Ctr_Type pv_ctr_type);
        ~Ctr();

        const char *get_name();

        friend class Ctr_Mgr;

    private:
        Ctr_Type        iv_ctr_type;
        struct timespec iv_ts_start;
        struct timespec iv_ts_stop;
    };

    class Ctr_Mgr {
    public:
        Ctr_Mgr();
        ~Ctr_Mgr();
        void acct(Ctr *pp_ctr);
        void report();

    private:
        typedef struct Stat_Type {
            SB_Thread::MSL iv_sl;
            SB_Int64_Type  iv_count;
            SB_Int64_Type  iv_time_min;
            SB_Int64_Type  iv_time_max;
            SB_Int64_Type  iv_time_total;
        } Stat_Type;

        Stat_Type       ia_stats[SB_ACTR_ZLAST];
        bool            iv_ready;
        bool            iv_reported;
        struct timeval  iv_ts_start;
        struct timeval  iv_ts_stop;
    };
#endif
}

#ifdef USE_SB_API_CTRS
extern SB_API::Ctr_Mgr gv_sb_api_ctr_mgr;
#endif

#ifdef USE_SB_API_CTRS
#define SB_API_CTR(var,ctr) SB_API::Ctr var(SB_API::SB_ACTR_ ## ctr)
#else
#define SB_API_CTR(var,ctr)
#endif

#ifdef USE_SB_API_CTRS
#include "apictr.inl"
#endif

#endif // !__SB_APICTR_H_
