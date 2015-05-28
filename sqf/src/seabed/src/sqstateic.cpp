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

//
// sqstate ms-ic
//

#include <dlfcn.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/sqstatehi.h"

#include "fsi.h"
#include "labelmapsx.h"
#ifndef SQ_PHANDLE_VERIFIER
#include "llmap.h"
#endif
#include "msicctr.h"
#include "msix.h"
#include "mslabelmapsx.h"
#include "msod.h"
#ifdef SQ_PHANDLE_VERIFIER
#include "npvmap.h"
#endif
#include "smap.h"
#include "sqstatesb.h"
#include "sqstateicvars.h"
#include "timerx.h"

enum { MAX_IC_ARGS = 100 };
enum { MAX_RSP     = 1024 * 1024 }; // 1MB

extern "C" {
    void sb_ic_call_test();
    void sb_ic_get_mds(BMS_SRE *sre);
    void sb_ic_get_metrics(BMS_SRE *sre);
    void sb_ic_get_openers(BMS_SRE *sre);
    void sb_ic_get_openers_bin(BMS_SRE *sre);
    void sb_ic_get_opens(BMS_SRE *sre);
    void sb_ic_get_opens_bin(BMS_SRE *sre);
    void sb_ic_get_pstate(BMS_SRE *sre);
    void sb_ic_prog(BMS_SRE *sre);
}

typedef struct Sqstate_Ic_Format_Desc {
    Sqstate_Ic_Struct_Fmt  iv_struct_formatter;
    void                  *ip_struct_desc;
} Sqstate_Ic_Format_Desc;

//
// simplify mon names
//
typedef MS_Mon_Process_Info_Type  Proc_Info_Type;
typedef MS_Mon_Node_Info_Type     Node_Info_Type;

extern SB_Table_Mgr<FS_Fd_Type>   gv_fs_filenum_table;
extern Ms_Od_Table_Mgr            gv_ms_od_mgr;

static char                      *ga_ic_argv[MAX_IC_ARGS];
static int                        gv_ic_argc;
static SB_Smap                   *gp_ic_struct_map = NULL;
static SB_Smap                   *gp_ic_var_map = NULL;

enum {
    MDS_ARG_OPT_BASIC       = 1,
    MDS_ARG_OPT_DELTA       = 2,
    MDS_ARG_OPT_DETAIL      = 3,
    MDS_ARG_OPT_TIMESTAMP   = 4
};
enum {
    PSTATE_ARG_OPT_BASIC    = 1,
    PSTATE_ARG_OPT_DETAIL   = 2,
    PSTATE_ARG_OPT_FILES    = 3,
    PSTATE_ARG_OPT_NEWS     = 4,
    PSTATE_ARG_OPT_RECURSE  = 5,
    PSTATE_ARG_OPT_STACK    = 6
};
Sqstate_Ic_Arg_Opt_Type ga_ic_mds_arg_options[] = {
    { "-basic",     MDS_ARG_OPT_BASIC      },
    { "-delta",     MDS_ARG_OPT_DELTA      },
    { "-detail",    MDS_ARG_OPT_DETAIL     },
    { "-timestamp", MDS_ARG_OPT_TIMESTAMP  },
    { NULL,         0                      },
};
Sqstate_Ic_Arg_Opt_Type ga_ic_pstate_arg_options[] = {
    { "-basic",   PSTATE_ARG_OPT_BASIC   },
    { "-detail",  PSTATE_ARG_OPT_DETAIL  },
    { "-files",   PSTATE_ARG_OPT_FILES   },
    { "-news",    PSTATE_ARG_OPT_NEWS    },
    { "-recurse", PSTATE_ARG_OPT_RECURSE },
    { "-stack",   PSTATE_ARG_OPT_STACK   },
    { NULL,       0                      },
};
static bool gv_ic_mds_opt_basic      = false;
static bool gv_ic_mds_opt_delta      = false;
static bool gv_ic_mds_opt_detail     = false;
static bool gv_ic_mds_opt_timestamp  = false;
static bool gv_ic_pstate_opt_basic   = false;
static bool gv_ic_pstate_opt_detail  = false;
static bool gv_ic_pstate_opt_files   = false;
static bool gv_ic_pstate_opt_news    = false;
static bool gv_ic_pstate_opt_recurse = false;
static bool gv_ic_pstate_opt_stack   = false;

#if __WORDSIZE == 64
  #define XXPF64    "%ld"
  #define XXPF64X   "%lx"
  #define XXPF64X16 "%016lx"
#else
  #define XXPF64    "%lld"
  #define XXPF64X   "%llx"
  #define XXPF64X16 "%016llx"
#endif

//
// forwards
//
static void sqstateic_var_add_check(Sqstate_Ic_Var *pp_entry,
                                    int             pv_inx,
                                    const char     *pp_str,
                                    const char     *pp_var,
                                    const char     *pp_table);

namespace SB_Trans {
    class SB_IC_Msg_Mgr {
    public:
        static int get_mds(char *pp_rsp, int pv_rsp_len);
        static int get_metrics(char *pp_rsp, int pv_rsp_len);
        static int get_openers(char *pp_rsp, bool pv_str);
        static int get_opens(char *pp_rsp, bool pv_str);
        static int get_pstate(char *pp_rsp, int pv_rsp_len);
        static int prog(char *pp_rsp, int pv_rsp_len);
        static int prog_call(char *pp_arg, char *pp_rsp, int pv_len);
        static int prog_get(char *pp_arg,
                            char *pp_rsp,
                            int   pv_len,
                            int   pv_fmt_type);
        static int prog_list(char *pp_rsp, int pv_len, bool pv_detail);
        static int prog_set(char *pp_arg, char *pp_rsp, int pv_len);

    private:
        typedef struct Pstate {
            struct iv_stat {
                unsigned long long  iv_btime;
            } iv_stat;
            struct iv_pidstat {
                char                ia_comm[1024];
                unsigned long       iv_cminflt;
                unsigned long       iv_cmajflt;
                unsigned long long  iv_cutime;
                unsigned long long  iv_cstime;
                unsigned long       iv_flags;
                long                iv_itrealvalue;
                unsigned long       iv_majflt;
                unsigned long       iv_minflt;
                long                iv_nice;
                long                iv_num_threads;
                int                 iv_pgrp;
                int                 iv_pid;
                int                 iv_ppid;
                long                iv_priority;
                long                iv_rss;
                int                 iv_session;
                unsigned long long  iv_starttime;
                char                iv_state;
                unsigned long long  iv_stime;
                int                 iv_tpgid;
                int                 iv_tty;
                unsigned long long  iv_utime;
                unsigned long       iv_vsize;
            } iv_pidstat;
            struct iv_pidstatus {
                char                ia_vmdata[10];
                char                ia_vmexe[10];
                char                ia_vmlib[10];
                char                ia_vmpeak[10];
                char                ia_vmrss[10];
                char                ia_vmsize[10];
                char                ia_vmstk[10];
                unsigned long long  iv_vmdata;
                unsigned long long  iv_vmexe;
                unsigned long long  iv_vmlib;
                unsigned long long  iv_vmpeak;
                unsigned long long  iv_vmrss;
                unsigned long long  iv_vmsize;
                unsigned long long  iv_vmstk;
            } iv_pidstatus;
            struct rusage           iv_rusage;
        } Pstate;
        static void  get_mds_fmt_ts(char           *pp_str,
                                    const char     *pp_label,
                                    struct timeval *pp_tv);
        static void  get_mds_min_max(uint64_t        *pp_min,
                                     uint64_t        *pp_max,
                                     const char     **ppp_min,
                                     const char     **ppp_max,
                                     const char      *pp_label,
                                     struct timeval  *pp_tv);
        static int   get_metric_fd_cap_count();
        static int   get_metric_fd_inuse_count();
        static int   get_metric_fd_hi_inuse_count();
        static int   get_metric_limq_count();
        static int   get_metric_limq_hi_count();
        static int   get_metric_md_curr_recv_count();
        static int   get_metric_md_curr_send_count();
        static int   get_metric_md_curr_total_count();
        static int   get_metric_md_hi_recv_count();
        static int   get_metric_md_hi_send_count();
        static int   get_metric_md_hi_total_count();
        static int   get_metric_md_max_recv_count();
        static int   get_metric_md_max_send_count();
        static int   get_metric_md_max_total_count();
        static int   get_metric_od_cap_count();
        static int   get_metric_od_inuse_count();
        static int   get_metric_od_hi_inuse_count();
        static int   get_metric_recvq_count();
        static int   get_metric_recvq_hi_count();
        static int   get_metric_stream_acc_count();
        static int   get_metric_stream_acc_hi_count();
        static int   get_metric_stream_con_count();
        static int   get_metric_stream_con_hi_count();
        static int   get_metric_stream_total_count();
        static int   get_metric_stream_total_hi_count();
        static int   get_metric_timer_count();
        static int   get_metric_timer_hi();
        static int   get_pstate_creator(char       *pp_rsp,
                                        const char *pp_indent,
                                        int         pv_nid,
                                        const char *pp_node_name,
                                        int         pv_pid,
                                        char       *pp_name,
                                        char       *pp_prog);
        static int   get_pstate_exec_time(char       *pp_rsp,
                                          const char *pp_indent,
                                          Pstate     *pp_s);
        static int   get_pstate_files(char *pp_rsp, const char *pp_indent2);
        static int   get_pstate_files_native(char       *pp_rsp,
                                             const char *pp_indent2);
        static int   get_pstate_libs(char *pp_rsp, const char *pp_indent);
        static int   get_pstate_memory(char       *pp_rsp,
                                       const char *pp_indent,
                                       const char *pp_indent2,
                                       Pstate     *pp_s);
        static int   get_pstate_nl(char *pp_rsp);
        static char *get_pstate_node(char *pp_node);
        static int   get_pstate_pri(char       *pp_rsp,
                                    const char *pp_indent,
                                    Pstate     *pp_s);
        static bool  get_pstate_pstate(pid_t pv_pid, Pstate *pp_s);
        static int   get_pstate_stack(char       *pp_rsp,
                                      const char *pp_indent,
                                      const char *pp_indent2);
        static int   get_pstate_target(char       *pp_rsp,
                                       const char *pp_indent,
                                       int         pv_nid,
                                       const char *pp_node_name,
                                       int         pv_pid,
                                       char       *pp_name,
                                       char       *pp_prog);
        static int   get_pstate_user(char *pp_rsp, const char *pp_indent);
        static void  resume_threads();
        static void  suspend_threads();
    };

    class Str_Map {
    public:
        Str_Map();
        ~Str_Map();

        const char *get(const char *pp_str);
        int         print(char *pp_str, const char *pp_indent);
        void        put(const char *pp_str);

    private:
        enum      { ADD_CAP = 10 };
        char      **ipp_str;
        int         iv_cap;
        int         iv_size;
    };

    Str_Map::Str_Map() : ipp_str(NULL), iv_cap(0), iv_size(0) {
    }

    Str_Map::~Str_Map() {
        char *lp_str;
        int   lv_inx;

        for (lv_inx = 0; lv_inx < iv_size; lv_inx++) {
            lp_str = ipp_str[lv_inx];
            free(lp_str);
        }
        if (ipp_str != NULL)
            delete [] ipp_str;
    }

    const char *Str_Map::get(const char *pp_str) {
        char *lp_str;
        int   lv_inx;

        for (lv_inx = 0; lv_inx < iv_size; lv_inx++) {
            lp_str = ipp_str[lv_inx];
            if (strcmp(lp_str, pp_str) == 0)
                return lp_str;
        }
        return NULL;
    }

    int Str_Map::print(char *pp_str, const char *pp_indent) {
        char *lp_str;
        int   lv_inx;
        int   lv_len;

        lv_len = 0;
        for (lv_inx = 0; lv_inx < iv_size; lv_inx++) {
            lp_str = ipp_str[lv_inx];
            lv_len += sprintf(&pp_str[lv_len], "%sloadfile: %s",
                              pp_indent, lp_str);
        }
        return lv_len;
    }

    void Str_Map::put(const char *pp_str) {
        char **lpp_str;
        int    lv_inx;
        int    lv_cap;

        if (iv_cap == 0) {
            iv_cap = ADD_CAP;
            ipp_str = new char *[iv_cap];
            for (lv_inx = 0; lv_inx < iv_cap; lv_inx++)
                ipp_str[lv_inx] = NULL;
        } else if (iv_size >= iv_cap) {
            lv_cap = iv_cap + ADD_CAP;
            lpp_str = new char *[lv_cap];
            for (lv_inx = 0; lv_inx < iv_cap; lv_inx++)
                lpp_str[lv_inx] = ipp_str[lv_inx];
            for (lv_inx = iv_cap; lv_inx < lv_cap; lv_inx++)
                lpp_str[lv_inx] = NULL;
            delete [] ipp_str;
            ipp_str = lpp_str;
            iv_cap = lv_cap;
        }
        ipp_str[iv_size] = strdup(pp_str);
        iv_size++;
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_mds(char *pp_rsp, int pv_rsp_len) {
        // 123456789012345
        // HH:MM:SS.UUUUUU
        char                   la_delta1[50];
        char                   la_delta2[50];
        char                   la_delta_c_break[50];
        char                   la_delta_c_link[50];
        char                   la_delta_c_rcvd[50];
        char                   la_delta_s_list[50];
        char                   la_delta_s_rcvd[50];
        char                   la_delta_s_reply[50];
        char                   la_delta_time[50];
        const char            *lp_delta_min;
        const char            *lp_delta_max;
        MS_Md_Type            *lp_md;
        SB_Trans::Stream_Base *lp_stream;
        const char            *lp_state_str;
        const char            *lp_stream_str;
        const char            *lp_where_str;
        int                    lv_arg;
        int                    lv_cap;
        uint64_t               lv_delta;
        uint64_t               lv_delta_cur;
        uint64_t               lv_delta_max;
        uint64_t               lv_delta_min;
        int                    lv_delta_sec;
        int                    lv_delta_usec;
        int                    lv_inx;
        int                    lv_len;
        int                    lv_msgid;
        int                    lv_rsp_len;
        struct timeval         lv_tv;

        lv_rsp_len = sqstateic_set_ic_args_options(gv_ic_argc,
                                                   ga_ic_argv,
                                                   pp_rsp,
                                                   pv_rsp_len,
                                                   ga_ic_mds_arg_options);
        if (lv_rsp_len)
            return lv_rsp_len;

        gv_ic_mds_opt_basic = false;
        gv_ic_mds_opt_delta = false;
        gv_ic_mds_opt_detail = false;
        gv_ic_mds_opt_timestamp = false;
        for (lv_arg = 0; lv_arg < gv_ic_argc; lv_arg++) {
            for (lv_inx = 0;
                 ga_ic_mds_arg_options[lv_inx].option_str != NULL;
                 lv_inx++) {
                if (strcmp(ga_ic_argv[lv_arg],
                           ga_ic_mds_arg_options[lv_inx].option_str) == 0) {
                    switch (ga_ic_mds_arg_options[lv_inx].option_int) {
                    case MDS_ARG_OPT_BASIC:
                        gv_ic_mds_opt_basic = true;
                        break;
                    case MDS_ARG_OPT_DELTA:
                        gv_ic_mds_opt_delta = true;
                        break;
                    case MDS_ARG_OPT_DETAIL:
                        gv_ic_mds_opt_detail = true;
                        break;
                    case MDS_ARG_OPT_TIMESTAMP:
                        gv_ic_mds_opt_timestamp = true;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        lv_len = 0;
        SB_Trans::Msg_Mgr::cv_md_table.lock();
        lv_cap = static_cast<int>(SB_Trans::Msg_Mgr::cv_md_table.get_cap());
        for (lv_msgid = 1; lv_msgid < lv_cap; lv_msgid++) {
            lp_md =
              SB_Trans::Msg_Mgr::cv_md_table.get_entry_lock(lv_msgid, false);
            if (lp_md->iv_inuse) {
                lp_stream =
                  static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                if (lp_stream != NULL)
                    lp_stream_str = lp_stream->get_name();
                else
                    lp_stream_str = "?";
                if (lp_md->ip_where != NULL)
                    lp_where_str = lp_md->ip_where;
                else
                    lp_where_str = "?";
                lp_state_str =
                  SB_get_label(&gv_sb_stream_md_state_label_map,
                               lp_md->iv_md_state);
                if (gv_ic_mds_opt_detail)
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "md[%d]=%p, stream=%s, where=%s, state=%d(%s), send-done=%d, slot-hdr=%d, ctrl=%d, data=%d\n",
                                       lv_msgid,
                                       pfp(lp_md),
                                       lp_stream_str,
                                       lp_where_str,
                                       lp_md->iv_md_state,
                                       lp_state_str,
                                       lp_md->iv_ss.iv_send_done,
                                       lp_md->iv_ss.iv_slot_hdr,
                                       lp_md->iv_ss.iv_slot_ctrl,
                                       lp_md->iv_ss.iv_slot_data);
                else if (gv_ic_mds_opt_timestamp) {
                    gettimeofday(&lv_tv, NULL);
                    get_mds_fmt_ts(la_delta_time, ", time=", &lv_tv);
                    get_mds_fmt_ts(la_delta_c_link,
                                   ", c-link=",
                                   &lp_md->iv_ts_msg_cli_link);
                    get_mds_fmt_ts(la_delta_s_rcvd,
                                   ", s-rcvd=",
                                   &lp_md->iv_ts_msg_srv_rcvd);
                    get_mds_fmt_ts(la_delta_s_list,
                                   ", s-listen=",
                                   &lp_md->iv_ts_msg_srv_listen);
                    get_mds_fmt_ts(la_delta_s_reply,
                                   ", s-reply=",
                                   &lp_md->iv_ts_msg_srv_reply);
                    get_mds_fmt_ts(la_delta_c_rcvd,
                                   ", c-rcvd=",
                                   &lp_md->iv_ts_msg_cli_rcvd);
                    get_mds_fmt_ts(la_delta_c_break,
                                   ", c-break=",
                                   &lp_md->iv_ts_msg_cli_break);
                    la_delta1[0] = '\0';
                    la_delta2[0] = '\0';
                    if (gv_ic_mds_opt_delta) {
                        lv_delta_min = static_cast<uint64_t>(INT_MAX) * 1000000;
                        lv_delta_max = 0;
                        lp_delta_min = "?";
                        lp_delta_max = "?";
                        get_mds_min_max(&lv_delta_min,
                                        &lv_delta_max,
                                        &lp_delta_min,
                                        &lp_delta_max,
                                        "c-link",
                                        &lp_md->iv_ts_msg_cli_link);
                        get_mds_min_max(&lv_delta_min,
                                        &lv_delta_max,
                                        &lp_delta_min,
                                        &lp_delta_max,
                                        "s-rcvd",
                                        &lp_md->iv_ts_msg_srv_rcvd);
                        get_mds_min_max(&lv_delta_min,
                                        &lv_delta_max,
                                        &lp_delta_min,
                                        &lp_delta_max,
                                        "s-listen",
                                        &lp_md->iv_ts_msg_srv_listen);
                        get_mds_min_max(&lv_delta_min,
                                        &lv_delta_max,
                                        &lp_delta_min,
                                        &lp_delta_max,
                                        "s-link",
                                        &lp_md->iv_ts_msg_srv_reply);
                        get_mds_min_max(&lv_delta_min,
                                        &lv_delta_max,
                                        &lp_delta_min,
                                        &lp_delta_max,
                                        "c-rcvd",
                                        &lp_md->iv_ts_msg_cli_rcvd);
                        get_mds_min_max(&lv_delta_min,
                                        &lv_delta_max,
                                        &lp_delta_min,
                                        &lp_delta_max,
                                        "c-break",
                                        &lp_md->iv_ts_msg_cli_break);
                        if (lv_delta_max != 0) {
                            lv_delta = lv_delta_max - lv_delta_min;
                            lv_delta_sec = static_cast<int>(lv_delta / 1000000);
                            lv_delta_usec = static_cast<int>(lv_delta - lv_delta_sec * 1000000);
                            sprintf(la_delta1, ", delta(%s-%s)=%d.%06d",
                                    lp_delta_min,
                                    lp_delta_max,
                                    lv_delta_sec,
                                    lv_delta_usec);
                            lv_delta_cur = static_cast<uint64_t>(lv_tv.tv_sec) * 1000000 + lv_tv.tv_usec;
                            lv_delta = lv_delta_cur - lv_delta_max;
                            lv_delta_sec = static_cast<int>(lv_delta / 1000000);
                            lv_delta_usec = static_cast<int>(lv_delta - lv_delta_sec * 1000000);
                            sprintf(la_delta2, ", delta(cur-%s)=%d.%06d",
                                    lp_delta_max,
                                    lv_delta_sec,
                                    lv_delta_usec);
                        }
                    }
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "md[%d]=%p, stream=%s, where=%s, state=%d(%s), reqid=%d%s%s%s%s%s%s%s%s%s\n",
                                       lv_msgid,
                                       pfp(lp_md),
                                       lp_stream_str,
                                       lp_where_str,
                                       lp_md->iv_md_state,
                                       lp_state_str,
                                       lp_md->out.iv_recv_req_id,
                                       la_delta_time,
                                       la_delta_c_link,
                                       la_delta_s_rcvd,
                                       la_delta_s_list,
                                       la_delta_s_reply,
                                       la_delta_c_rcvd,
                                       la_delta_c_break,
                                       la_delta1,
                                       la_delta2);
                } else
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "md[%d]=%p, stream=%s, where=%s, state=%d(%s)\n",
                                       lv_msgid,
                                       pfp(lp_md),
                                       lp_stream_str,
                                       lp_where_str,
                                       lp_md->iv_md_state,
                                       lp_state_str);
            }
        }
        SB_Trans::Msg_Mgr::cv_md_table.unlock();

        return lv_len;
    }

    void SB_IC_Msg_Mgr::get_mds_fmt_ts(char           *pp_str,
                                       const char     *pp_label,
                                       struct timeval *pp_tv) {
        int lv_hr;
        int lv_min;
        int lv_sec;

        if (pp_tv->tv_sec == 0)
            *pp_str = '\0';
        else {
            lv_sec = static_cast<int>(pp_tv->tv_sec) % 86400;
            lv_hr = lv_sec / 3600;
            lv_sec %= 3600;
            lv_min = lv_sec / 60;
            lv_sec = lv_sec % 60;
            sprintf(pp_str, "%s%02d:%02d:%02d.%06ld",
                    pp_label, lv_hr, lv_min, lv_sec, pp_tv->tv_usec);
        }
    }

    void SB_IC_Msg_Mgr::get_mds_min_max(uint64_t        *pp_min,
                                        uint64_t        *pp_max,
                                        const char     **ppp_min,
                                        const char     **ppp_max,
                                        const char      *pp_label,
                                        struct timeval  *pp_tv) {
        uint64_t lv_sec;
        uint64_t lv_ts;

        lv_sec = pp_tv->tv_sec;
        if (lv_sec != 0) {
            lv_ts = lv_sec * 1000000 + pp_tv->tv_usec;
            if (lv_ts < *pp_min) {
                *pp_min = lv_ts;
                *ppp_min = pp_label;
            }
            if (lv_ts > *pp_max) {
                *pp_max = lv_ts;
                *ppp_max = pp_label;
            }
        }
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metrics(char *pp_rsp, int pv_rsp_len) {
        int lv_len;

        pv_rsp_len = pv_rsp_len; // touch

        lv_len = 0;
        lv_len += sprintf(&pp_rsp[lv_len], "Metrics\n");
        lv_len += sprintf(&pp_rsp[lv_len], "  FD cap=%d, inuse=%d, inuse-high=%d\n",
                          get_metric_fd_cap_count(),
                          get_metric_fd_inuse_count(),
                          get_metric_fd_hi_inuse_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  MD send=%d, recv=%d, total=%d\n",
                          get_metric_md_curr_send_count(),
                          get_metric_md_curr_recv_count(),
                          get_metric_md_curr_total_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  MD send-high=%d, recv-high=%d, total-high=%d\n",
                          get_metric_md_hi_send_count(),
                          get_metric_md_hi_recv_count(),
                          get_metric_md_hi_total_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  MD send-max=%d, recv-max=%d, total-max=%d\n",
                          get_metric_md_max_send_count(),
                          get_metric_md_max_recv_count(),
                          get_metric_md_max_total_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  OD cap=%d, inuse=%d, inuse-high=%d\n",
                          get_metric_od_cap_count(),
                          get_metric_od_inuse_count(),
                          get_metric_od_hi_inuse_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  RECEIVE queue length=%d, high=%d\n",
                          get_metric_recvq_count(),
                          get_metric_recvq_hi_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  RECEIVE-LIMIT queue length=%d, high=%d\n",
                          get_metric_limq_count(),
                          get_metric_limq_hi_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  Stream accept=%d, connect=%d, total=%d\n",
                          get_metric_stream_acc_count(),
                          get_metric_stream_con_count(),
                          get_metric_stream_total_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  Stream accept-high=%d, connect-high=%d, total-high=%d\n",
                          get_metric_stream_acc_hi_count(),
                          get_metric_stream_con_hi_count(),
                          get_metric_stream_total_hi_count());
        lv_len += sprintf(&pp_rsp[lv_len], "  Timer count=%d, high=%d\n",
                          get_metric_timer_count(),
                          get_metric_timer_hi());

        return lv_len;
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_openers(char *pp_rsp, bool pv_str) {
#ifdef SQ_PHANDLE_VERIFIER
        SB_NPVmap_Enum           *lp_enum;
#else
        SB_LLmap_Enum            *lp_enum;
#endif
        SB_IC_Get_Opener_Type    *lp_rsp_bin;
        SB_IC_Get_Opener_Od_Type *lp_rsp_od;
        SB_Trans::Trans_Stream   *lp_stream;
        int                       lv_count;
        int                       lv_len;
        int                       lv_rsp_len;

        SB_Trans::Trans_Stream::map_nidpid_lock();

        lv_count = SB_Trans::Trans_Stream::map_nidpid_size();
        lp_enum = SB_Trans::Trans_Stream::map_nidpid_keys();
        lv_rsp_len = 0;
        lp_rsp_bin = reinterpret_cast<SB_IC_Get_Opener_Type *>(pp_rsp);
        lp_rsp_od = lp_rsp_bin->ia_od;
        if (pv_str) {
            lv_len = sprintf(&pp_rsp[lv_rsp_len], "opener-count=%d\n", lv_count);
            lv_rsp_len += lv_len;
        } else {
            lp_rsp_bin->iv_count = lv_count;
            lv_rsp_len = sizeof(lp_rsp_bin->iv_count);
        }

        while (lp_enum->more()) {
#ifdef SQ_PHANDLE_VERIFIER
            lp_stream = SB_Trans::Trans_Stream::map_nidpid_key_to_stream(lp_enum->next()->iv_id.npv, false);
#else
            lp_stream = SB_Trans::Trans_Stream::map_nidpid_key_to_stream(lp_enum->next()->iv_id.ll, false);
#endif
            if (pv_str) {
                lv_len = sprintf(&pp_rsp[lv_rsp_len],
                                "stream=%p %s\n",
                                pfp(lp_stream),
                                lp_stream->get_name());
                lv_rsp_len += lv_len;
            } else {
                strcpy(lp_rsp_od->ia_name, lp_stream->get_name());
                lp_rsp_od->iv_nid = lp_stream->get_remote_nid();
                lp_rsp_od->iv_pid = lp_stream->get_remote_pid();
#ifdef SQ_PHANDLE_VERIFIER
                lp_rsp_od->iv_verif = lp_stream->get_remote_verif();
#endif
                lp_rsp_od++;
                lv_rsp_len += static_cast<int>(sizeof(*lp_rsp_od));
            }
        }
        delete lp_enum;

        SB_Trans::Trans_Stream::map_nidpid_unlock();

        if (pv_str) {
            pp_rsp[lv_rsp_len] = '\0'; // terminate
            lv_rsp_len++;
        }
        return lv_rsp_len;
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_opens(char *pp_rsp, bool pv_str) {
        Ms_Od_Type              *lp_od;
        SB_IC_Get_Opens_Type    *lp_rsp_bin;
        SB_IC_Get_Opens_Od_Type *lp_rsp_od;
        int                      lv_cap;
        int                      lv_inuse;
        int                      lv_len;
        int                      lv_oid;
        int                      lv_rsp_len;

        lv_cap = static_cast<int>(gv_ms_od_mgr.get_cap());
        lv_inuse = static_cast<int>(gv_ms_od_mgr.get_inuse());
        if (lv_inuse > 0)
            lv_inuse--;
        lv_rsp_len = 0;
        lp_rsp_bin = reinterpret_cast<SB_IC_Get_Opens_Type *>(pp_rsp);
        lp_rsp_od = lp_rsp_bin->ia_od;
        if (pv_str) {
            lv_len = sprintf(&pp_rsp[lv_rsp_len], "open-count=%d\n", lv_inuse);
            lv_rsp_len += lv_len;
        } else {
            lp_rsp_bin->iv_count = lv_inuse;
            lv_rsp_len = sizeof(lp_rsp_bin->iv_count);
        }
        for (lv_oid = 1; lv_oid < lv_cap; lv_oid++) {
            lp_od = gv_ms_od_mgr.get_entry_lock(lv_oid, false);
            if (lp_od == NULL)
                continue;
            if (lp_od->iv_inuse) {
                if (pv_str) {
                    if (lp_od->iv_self)
                        lv_len = sprintf(&pp_rsp[lv_rsp_len],
                                        "od[%d]=self\n", lv_oid);
                    else if (lp_od->ip_stream != NULL)
                        lv_len = sprintf(&pp_rsp[lv_rsp_len],
#ifdef SQ_PHANDLE_VERIFIER
                                        "od[%d]=%s-%s, p-id=%d/%d" PFVY ", ref=%d\n",
#else
                                        "od[%d]=%s-%s, p-id=%d/%d, ref=%d\n",
#endif
                                        lv_oid,
                                        lp_od->ia_process_name,
                                        lp_od->ia_prog,
                                        lp_od->iv_nid,
                                        lp_od->iv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                        lp_od->iv_verif,
#endif
                                        lp_od->iv_ref_count);
                    else
                        lv_len = sprintf(&pp_rsp[lv_rsp_len],
                                        "od[%d]=inprogress\n", lv_oid);
                    lv_rsp_len += lv_len;
                } else {
                    strcpy(lp_rsp_od->ia_process_name, lp_od->ia_process_name);
                    strcpy(lp_rsp_od->ia_prog, lp_od->ia_prog);
                    lp_rsp_od->iv_nid = lp_od->iv_nid;
                    lp_rsp_od->iv_pid = lp_od->iv_pid;
#ifdef SQ_PHANDLE_VERIFIER
                    lp_rsp_od->iv_verif = lp_od->iv_verif;
#endif
                    lp_rsp_od++;
                    lv_rsp_len += static_cast<int>(sizeof(*lp_rsp_od));
                }
            }
        }
        if (pv_str) {
            pp_rsp[lv_rsp_len] = '\0'; // terminate
            lv_rsp_len++;
        }
        return lv_rsp_len;
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_fd_cap_count() {
        return static_cast<int>(gv_fs_filenum_table.get_cap());
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_fd_inuse_count() {
        return static_cast<int>(gv_fs_filenum_table.get_inuse());
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_fd_hi_inuse_count() {
        return static_cast<int>(gv_fs_filenum_table.get_hi_inuse());
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_limq_count() {
        return gv_ms_ic_ctr.ctr_get_limq_len();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_limq_hi_count() {
        return gv_ms_ic_ctr.ctr_get_limq_hi();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_curr_recv_count() {
        return SB_Trans::Msg_Mgr::get_md_count_recv();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_curr_send_count() {
        return SB_Trans::Msg_Mgr::get_md_count_send();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_curr_total_count() {
        return SB_Trans::Msg_Mgr::get_md_count_total();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_hi_recv_count() {
        return SB_Trans::Msg_Mgr::get_md_hi_recv();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_hi_send_count() {
        return SB_Trans::Msg_Mgr::get_md_hi_send();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_hi_total_count() {
        return SB_Trans::Msg_Mgr::get_md_hi_total();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_max_recv_count() {
        return SB_Trans::Msg_Mgr::get_md_max_recv();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_max_send_count() {
        return SB_Trans::Msg_Mgr::get_md_max_send();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_md_max_total_count() {
        return SB_Trans::Msg_Mgr::get_md_max_total();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_od_cap_count() {
        return static_cast<int>(gv_ms_od_mgr.get_cap());
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_od_inuse_count() {
        return static_cast<int>(gv_ms_od_mgr.get_inuse());
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_od_hi_inuse_count() {
        return static_cast<int>(gv_ms_od_mgr.get_hi_inuse());
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_recvq_count() {
        return gv_ms_ic_ctr.ctr_get_recvq_len();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_recvq_hi_count() {
        return gv_ms_ic_ctr.ctr_get_recvq_hi();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_stream_acc_count() {
        return Trans_Stream::get_stream_acc_count();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_stream_acc_hi_count() {
        return Trans_Stream::get_stream_acc_hi_count();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_stream_con_count() {
        return Trans_Stream::get_stream_con_count();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_stream_con_hi_count() {
        return Trans_Stream::get_stream_con_hi_count();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_stream_total_count() {
        return Trans_Stream::get_stream_total_count();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_stream_total_hi_count() {
        return Trans_Stream::get_stream_total_hi_count();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_timer_count() {
        return sb_timer_size();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_metric_timer_hi() {
        return sb_timer_hi();
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::get_pstate(char *pp_rsp, int pv_rsp_len) {
        char            la_name[MS_MON_MAX_PROCESS_NAME];
        char           *lp_node_name;
        char           *lp_prog;
        int             lv_arg;
        int             lv_err;
        int             lv_inx;
        int             lv_nid;
        int             lv_nid_parent;
        Node_Info_Type  lv_node_info;
        Node_Info_Type  lv_node_info_parent;
        bool            lv_parent_ok;
        int             lv_pid;
        int             lv_pid_parent;
        int             lv_pnid;
        Proc_Info_Type  lv_proc_info;
        Proc_Info_Type  lv_proc_info_parent;
        Pstate          lv_pstate;
        bool            lv_pstate_ok;
        int             lv_ptype;
        int             lv_rsp_len;

        lv_rsp_len = sqstateic_set_ic_args_options(gv_ic_argc,
                                                   ga_ic_argv,
                                                   pp_rsp,
                                                   pv_rsp_len,
                                                   ga_ic_pstate_arg_options);
        if (lv_rsp_len)
            return lv_rsp_len;

        gv_ic_pstate_opt_basic = false;
        gv_ic_pstate_opt_detail = false;
        gv_ic_pstate_opt_files = false;
        gv_ic_pstate_opt_news = false;
        gv_ic_pstate_opt_recurse = false;
        gv_ic_pstate_opt_stack = false;
        for (lv_arg = 0; lv_arg < gv_ic_argc; lv_arg++) {
            for (lv_inx = 0;
                 ga_ic_pstate_arg_options[lv_inx].option_str != NULL;
                 lv_inx++) {
                if (strcmp(ga_ic_argv[lv_arg],
                           ga_ic_pstate_arg_options[lv_inx].option_str) == 0) {
                    switch (ga_ic_pstate_arg_options[lv_inx].option_int) {
                    case PSTATE_ARG_OPT_BASIC:
                        gv_ic_pstate_opt_basic = true;
                        break;
                    case PSTATE_ARG_OPT_DETAIL:
                        gv_ic_pstate_opt_detail = true;
                        break;
                    case PSTATE_ARG_OPT_FILES:
                        gv_ic_pstate_opt_files = true;
                        break;
                    case PSTATE_ARG_OPT_NEWS:
                        gv_ic_pstate_opt_news = true;
                        break;
                    case PSTATE_ARG_OPT_RECURSE:
                        gv_ic_pstate_opt_recurse = true;
                        break;
                    case PSTATE_ARG_OPT_STACK:
                        gv_ic_pstate_opt_stack = true;
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        if (!(gv_ic_pstate_opt_basic ||
            gv_ic_pstate_opt_files ||
            gv_ic_pstate_opt_detail ||
            gv_ic_pstate_opt_stack)) {
            gv_ic_pstate_opt_files = true;
            gv_ic_pstate_opt_stack = true;
        }

        lv_rsp_len = 0;
        lv_err = msg_mon_get_my_info3(&lv_nid,                 // nid
                                      NULL,                    // pid
                                      la_name,                 // name
                                      MS_MON_MAX_PROCESS_NAME, // name-len
                                      &lv_ptype,               // ptype
                                      NULL,                    // zid
                                      &lv_pid,                 // os-pid
                                      NULL,                    // os-tid
                                      NULL,                    // compid
                                      &lv_pnid);               // pnid
        SB_util_assert_ieq(lv_err, XZFIL_ERR_OK);
        lv_err = msg_mon_get_node_info_detail(lv_nid, &lv_node_info);
        SB_util_assert_ieq(lv_err, XZFIL_ERR_OK);
        lv_err = msg_mon_get_process_info_detail(la_name, &lv_proc_info);
        SB_util_assert_ieq(lv_err, XZFIL_ERR_OK);
        lv_err = msg_mon_get_process_info_detail(lv_proc_info.parent_name,
                                                 &lv_proc_info_parent);
        lv_parent_ok = (lv_err == XZFIL_ERR_OK);
        if (lv_parent_ok) {
            lv_nid_parent = lv_proc_info.parent_nid;
            lv_pid_parent = lv_proc_info.parent_pid;
            if (lv_nid_parent == lv_nid)
                memcpy(&lv_node_info_parent,
                       &lv_node_info,
                       sizeof(lv_node_info));
            else {
                lv_err = msg_mon_get_node_info_detail(lv_nid_parent,
                                                      &lv_node_info_parent);
                SB_util_assert_ieq(lv_err, XZFIL_ERR_OK);
            }
        } else {
            lv_nid_parent = 0;
            lv_pid_parent = 0;
        }

        lv_pstate_ok = get_pstate_pstate(lv_pid, &lv_pstate);

        lp_node_name =
          get_pstate_node(lv_node_info.node[0].node_name);
        lp_prog = basename(lv_proc_info.program);
        lv_rsp_len += get_pstate_target(&pp_rsp[lv_rsp_len],
                                        "",
                                        lv_nid,
                                        lp_node_name,
                                        lv_pid,
                                        la_name,
                                        lp_prog);

        if (gv_ic_pstate_opt_detail) {
            lv_rsp_len += get_pstate_libs(&pp_rsp[lv_rsp_len], "  ");
            lv_rsp_len += get_pstate_nl(&pp_rsp[lv_rsp_len]);
        }

        if (lv_pstate_ok)
            lv_rsp_len += get_pstate_pri(&pp_rsp[lv_rsp_len],
                                         " ",
                                         &lv_pstate);

        lv_rsp_len += get_pstate_nl(&pp_rsp[lv_rsp_len]);
        lv_rsp_len += get_pstate_user(&pp_rsp[lv_rsp_len], " ");

        lv_rsp_len += get_pstate_nl(&pp_rsp[lv_rsp_len]);

        if (lv_parent_ok) {
            lp_node_name =
              get_pstate_node(lv_node_info_parent.node[0].node_name);
            lp_prog = basename(lv_proc_info_parent.program);
            lv_rsp_len += get_pstate_creator(&pp_rsp[lv_rsp_len],
                                             " ",
                                             lv_nid_parent,
                                             lp_node_name,
                                             lv_pid_parent,
                                             lv_proc_info_parent.process_name,
                                             lp_prog);
        }

        if (lv_pstate_ok)
            lv_rsp_len += get_pstate_exec_time(&pp_rsp[lv_rsp_len],
                                               " ",
                                               &lv_pstate);

        if (gv_ic_pstate_opt_stack)
            lv_rsp_len += get_pstate_stack(&pp_rsp[lv_rsp_len],
                                           " ",
                                           "  ");

        if (lv_pstate_ok)
            lv_rsp_len += get_pstate_memory(&pp_rsp[lv_rsp_len],
                                            " ",
                                            "  ",
                                            &lv_pstate);

        if (gv_ic_pstate_opt_files) {
            lv_rsp_len += get_pstate_files(&pp_rsp[lv_rsp_len], " ");
            lv_rsp_len += get_pstate_files_native(&pp_rsp[lv_rsp_len], " ");
        }

        lv_rsp_len += get_pstate_nl(&pp_rsp[lv_rsp_len]);

        return lv_rsp_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_creator(char       *pp_rsp,
                                          const char *pp_indent,
                                          int         pv_nid,
                                          const char *pp_node_name,
                                          int         pv_pid,
                                          char       *pp_name,
                                          char       *pp_prog) {
        char  *lp_prog;
        int    lv_len;
        pid_t  lv_pid;
        Pstate lv_pstate;

        if ((pv_nid < 0) && (pv_pid < 0)) {
            lv_pid = getppid();
            if (get_pstate_pstate(lv_pid, &lv_pstate))
                lp_prog = basename(lv_pstate.iv_pidstat.ia_comm);
            else
                lp_prog = const_cast<char *>("?");
            lv_len = sprintf(pp_rsp,
                             "%screator process: pid %d, program %s\n",
                             pp_indent,
                             lv_pid,
                             lp_prog);
        } else {
            lv_len = sprintf(pp_rsp,
                             "%screator process: pid %d in node %s[nid %d], name %s, program %s\n",
                             pp_indent,
                             pv_pid,
                             pp_node_name,
                             pv_nid,
                             pp_name,
                             pp_prog);
        }
        return lv_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_exec_time(char       *pp_rsp,
                                            const char *pp_indent,
                                            Pstate     *pp_s) {
        char                la_month[50];
        char                la_time[50];
        struct tm          *lp_tm;
        unsigned long long  lv_exe;
        int                 lv_exe_usec;
        int                 lv_exe_sec;
        int                 lv_exe_min;
        int                 lv_exe_hrs;
        int                 lv_hz;
        int                 lv_len;
        time_t              lv_sec;
        time_t              lv_time;
        struct tm           lv_tm;

        lv_sec = static_cast<time_t>(pp_s->iv_pidstat.iv_starttime / HZ);
        lv_hz = static_cast<int>(pp_s->iv_pidstat.iv_starttime - lv_sec * HZ);
        lv_time = static_cast<time_t>(pp_s->iv_stat.iv_btime + lv_sec);
        lp_tm = localtime_r(&lv_time, &lv_tm);
        strftime(la_month, sizeof(la_month), "%B", lp_tm);
        sprintf(la_time, "%s %d, %d %02d:%02d:%02d.%02d (LCT)",
                la_month, lp_tm->tm_mday, lp_tm->tm_year + 1900,
                lp_tm->tm_hour, lp_tm->tm_min, lp_tm->tm_sec, lv_hz);
        lv_len = sprintf(pp_rsp, "%sprocess creation time: %s\n",
                         pp_indent, la_time);
        lv_exe = pp_s->iv_rusage.ru_utime.tv_sec * 1000000 +
                 pp_s->iv_rusage.ru_utime.tv_usec +
                 pp_s->iv_rusage.ru_stime.tv_sec * 1000000 +
                 pp_s->iv_rusage.ru_stime.tv_usec;
        lv_exe_usec = static_cast<int>(lv_exe % 1000000);
        lv_exe -= lv_exe_usec;
        lv_exe /= 1000000;
        lv_exe_sec = static_cast<int>(lv_exe % 60);
        lv_exe -= lv_exe_sec;
        lv_exe /= 60;
        lv_exe_min = static_cast<int>(lv_exe % 60);
        lv_exe -= lv_exe_min;
        lv_exe /= 60;
        lv_exe_hrs = static_cast<int>(lv_exe % 24);

        sprintf(la_time, "%02d:%02d:%02d.%06d",
                lv_exe_hrs, lv_exe_min, lv_exe_sec, lv_exe_usec);
        lv_len += sprintf(&pp_rsp[lv_len], "%sprocess execution time: %s\n",
                          pp_indent, la_time);
        return lv_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_libs(char *pp_rsp, const char *pp_indent) {
        char     la_line[BUFSIZ];
        FILE    *lp_file_map;
        char    *lp_s;
        char    *lp_slash;
        int      lv_len;
        Str_Map  lv_lib_map;

        lp_file_map = fopen("/proc/self/maps", "r");
        if (lp_file_map != NULL) {
            for (;;) {
                lp_s = fgets(la_line, sizeof(la_line), lp_file_map);
                if (lp_s == NULL)
                    break;
                lp_slash = strchr(lp_s, '/');
                if (lp_slash != NULL) {
                    if (strstr(lp_slash, "(deleted)") == NULL) {
                        if (lv_lib_map.get(lp_slash) == NULL)
                            lv_lib_map.put(lp_slash);
                    }
                }
            }
            fclose(lp_file_map);
            lv_len = lv_lib_map.print(pp_rsp, pp_indent);
        } else
            lv_len = 0;
        return lv_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_files(char *pp_rsp, const char *pp_indent) {
        const char *INDENT = "          ";
        FS_Fd_Type *lp_fd;
        const char *lp_file_type;
        FS_Io_Type *lp_io;
        const char *lp_io_type;
        int         lv_filenum;
        int         lv_len;
        int         lv_io;
        bool        lv_prev;
        int         lv_size;

        suspend_threads();
        lv_size = static_cast<int>(gv_fs_filenum_table.get_inuse());
        if ((lv_size == 0) ||
            ((lv_size == 1) && (gv_fs_filenum_table.get_entry(0) != NULL))) {
            // empty
            lv_len = 0;
        } else {
            lv_len = sprintf(pp_rsp, "\n");
            lv_len += sprintf(&pp_rsp[lv_len],
                              "%sFnum  Filename              Type\n",
                              pp_indent);
            lv_len += sprintf(&pp_rsp[lv_len],
                          "%s----- --------------------- --------\n",
                          pp_indent);
            lv_len += sprintf(&pp_rsp[lv_len], "\n");
        }

        lv_size = static_cast<int>(gv_fs_filenum_table.get_cap());
        lv_prev = false;
        for (lv_filenum = 1; lv_filenum < lv_size; lv_filenum++) {
            lp_fd = gv_fs_filenum_table.get_entry(lv_filenum);
            if (lp_fd != NULL) {
                if (lv_prev)
                    lv_len += sprintf(&pp_rsp[lv_len], "\n");
                lv_prev = true;
                switch (lp_fd->iv_file_type) {
                case FS_FILE_TYPE_PROCESS:
                    lp_file_type = "Process";
                    break;
                case FS_FILE_TYPE_RECEIVE:
                    lp_file_type = "$RECEIVE";
                    break;
                default:
                    lp_file_type = "?";
                    break;
                }
                lv_len += sprintf(&pp_rsp[lv_len], "%s%5d %-21s %-10s\n",
                                  pp_indent,
                                  lv_filenum,
                                  lp_fd->ia_fname,
                                  lp_file_type);

                switch (lp_fd->iv_file_type) {
                case FS_FILE_TYPE_PROCESS:
                    if (lp_fd->iv_op_depth) {
                        lv_len += sprintf(&pp_rsp[lv_len],
                                          "%s%s%d outstanding I/Os.\n",
                                          pp_indent,
                                          INDENT,
                                          lp_fd->iv_op_depth);
                        lp_io = lp_fd->ip_io_old;
                        for (lv_io = 0; lv_io < lp_fd->iv_op_depth; lv_io++) {
                            if (lp_io == NULL)
                                break;
                            switch (lp_io->iv_io_type) {
                            case FS_FS_WRITE:
                                lp_io_type = "write";
                                break;
                            case FS_FS_WRITEREAD:
                                lp_io_type = "writeread";
                                break;
                            default:
                                lp_io_type = "<unknown>";
                                break;
                            }
                            lv_len += sprintf(&pp_rsp[lv_len],
                                              "%s%s   %s(%d)\n",
                                              pp_indent,
                                              INDENT,
                                              lp_io_type,
                                              lp_io->iv_io_type);
                            lp_io = lp_io->ip_io_next;
                        }
                    } else
                        lv_len += sprintf(&pp_rsp[lv_len],
                                          "%s%sNo outstanding I/Os.\n",
                                          pp_indent,
                                          INDENT);
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "%s%snowait depth is %d.\n",
                                      pp_indent,
                                      INDENT,
                                      lp_fd->iv_nowait_depth);
                    break;
                case FS_FILE_TYPE_RECEIVE:
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "%s%s$RECEIVE nowait depth is %d.\n",
                                      pp_indent,
                                      INDENT,
                                      lp_fd->iv_nowait_depth);
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "%s%s$RECEIVE receive depth is %d.\n",
                                      pp_indent,
                                      INDENT,
                                      lp_fd->iv_recv_depth);
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "%s%s$RECEIVE queue length: not yet read %d\n",
                                      pp_indent,
                                      INDENT,
                                      gv_ms_ic_ctr.ctr_get_recvq_len());
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "%s%sLIMIT-RECEIVE queue length: not yet read %d\n",
                                      pp_indent,
                                      INDENT,
                                      gv_ms_ic_ctr.ctr_get_limq_len());
                    lv_len += sprintf(&pp_rsp[lv_len],
                                      "%s%s                     : read but not REPLYed %d\n",
                                      pp_indent,
                                      INDENT,
                                      lp_fd->ip_ru_tag_mgr->size());
                    break;
                default:
                    break;
                }

                lv_len += sprintf(&pp_rsp[lv_len],
                                  "%s%sOptions at open time were 0x%04x.\n",
                                  pp_indent,
                                  INDENT,
                                  lp_fd->iv_options);
            }
        }

        resume_threads();

        return lv_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_files_native(char       *pp_rsp,
                                               const char *pp_indent) {
        char  la_line[BUFSIZ];
        char  la_sys[PATH_MAX];
        FILE *lp_file;
        char *lp_s;
        int   lv_len;

        lv_len = sprintf(pp_rsp, "\n");
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%sopen files\n",
                          pp_indent);
        sprintf(la_sys, "lsof -p %d", getpid());
        lp_file = popen(la_sys, "r");
        if (lp_file != NULL) {
            for (;;) {
                lp_s = fgets(la_line, sizeof(la_line), lp_file);
                if (lp_s == NULL)
                    break;
                lv_len += sprintf(&pp_rsp[lv_len],
                                  "%s%s%s",
                                  pp_indent,
                                  "  ",
                                  lp_s);
            }
            pclose(lp_file);
        }

        return lv_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_memory(char       *pp_rsp,
                                         const char *pp_indent,
                                         const char *pp_indent2,
                                         Pstate     *pp_s) {

        int                lv_len;
        unsigned long long lv_rcvd;
        unsigned long long lv_sent;

        lv_len = sprintf(pp_rsp,
                         "%sprocess memory values and other counters\n",
                         pp_indent);
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%sVirtual memory Stack Size (VmStk): %llu %s\n",
                          pp_indent,
                          pp_indent2,
                          pp_s->iv_pidstatus.iv_vmstk,
                          pp_s->iv_pidstatus.ia_vmstk);
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%sVirtual memory Heap Size (VmData): %llu %s\n",
                          pp_indent,
                          pp_indent2,
                          pp_s->iv_pidstatus.iv_vmdata,
                          pp_s->iv_pidstatus.ia_vmdata);
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%sVirtual memory Resident Set Size (VmRSS): %llu %s\n",
                          pp_indent,
                          pp_indent2,
                          pp_s->iv_pidstatus.iv_vmrss,
                          pp_s->iv_pidstatus.ia_vmrss);
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%sVirtual memory Size (VmSize): %llu %s\n",
                          pp_indent,
                          pp_indent2,
                          pp_s->iv_pidstatus.iv_vmsize,
                          pp_s->iv_pidstatus.ia_vmsize);
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%sVirtual memory Peak Size (VmPeak): %llu %s\n",
                          pp_indent,
                          pp_indent2,
                          pp_s->iv_pidstatus.iv_vmpeak,
                          pp_s->iv_pidstatus.ia_vmpeak);
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%sPage faults: %lu\n",
                          pp_indent,
                          pp_indent2,
                          pp_s->iv_pidstat.iv_majflt);
        lv_rcvd = gv_ms_ic_ctr.ctr_get_msgs_rcvd();
        lv_sent = gv_ms_ic_ctr.ctr_get_msgs_sent();
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%sMessages: Sent %llu, Received %llu\n",
                          pp_indent,
                          pp_indent2,
                          lv_sent,
                          lv_rcvd);
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%s$RECEIVE current Q-length: %d\n",
                          pp_indent,
                          pp_indent2,
                          gv_ms_ic_ctr.ctr_get_recvq_len());
        lv_len += sprintf(&pp_rsp[lv_len],
                          "%s%sLIMIT-RECEIVE current Q-length: %d\n",
                          pp_indent,
                          pp_indent2,
                          gv_ms_ic_ctr.ctr_get_limq_len());
        return lv_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_nl(char *pp_rsp) {
        int lv_len;

        lv_len = sprintf(pp_rsp, "\n");
        return lv_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_stack(char       *pp_rsp,
                                        const char *pp_indent,
                                        const char *pp_indent2) {
        char  la_line[BUFSIZ];
        char  la_sys[PATH_MAX];
        FILE *lp_file;
        char *lp_s;
        int   lv_len;

        lv_len = sprintf(pp_rsp,
                         "%scall stack\n",
                         pp_indent);
        sprintf(la_sys, "sqstatepstack %d", getpid());
        lp_file = popen(la_sys, "r");
        if (lp_file != NULL) {
            for (;;) {
                lp_s = fgets(la_line, sizeof(la_line), lp_file);
                if (lp_s == NULL)
                    break;
                lv_len += sprintf(&pp_rsp[lv_len],
                                  "%s%s%s",
                                  pp_indent,
                                  pp_indent2,
                                  lp_s);
            }
            pclose(lp_file);
        }
        return lv_len;
    }

    int SB_IC_Msg_Mgr::get_pstate_target(char       *pp_rsp,
                                         const char *pp_indent,
                                         int         pv_nid,
                                         const char *pp_node_name,
                                         int         pv_pid,
                                         char       *pp_name,
                                         char       *pp_prog) {
        int lv_len;

        lv_len =
          sprintf(pp_rsp,
                  "%starget process: pid %d in node %s[nid %d], name %s, program %s\n",
                  pp_indent,
                  pv_pid,
                  pp_node_name,
                  pv_nid,
                  pp_name,
                  pp_prog);
        return lv_len;
    }

    char *SB_IC_Msg_Mgr::get_pstate_node(char *pp_node) {
        char *lp_colon;
        char *lp_dot;

        lp_dot = strchr(pp_node, '.');
        lp_colon = strchr(pp_node, ':');
        if (lp_dot != NULL) {
            if (lp_colon != NULL)
                strcpy(lp_dot, lp_colon);
            else
                *lp_dot = '\0';
        }
        return pp_node;
    }

    int SB_IC_Msg_Mgr::get_pstate_pri(char       *pp_rsp,
                                      const char *pp_indent,
                                      Pstate     *pp_s) {
        int lv_len;

        lv_len = sprintf(pp_rsp, "%spriority is %ld, nice is %ld\n",
                         pp_indent,
                         pp_s->iv_pidstat.iv_priority,
                         pp_s->iv_pidstat.iv_nice);
        return lv_len;
    }

    bool SB_IC_Msg_Mgr::get_pstate_pstate(int pv_pid, Pstate *pp_s) {
        char  la_file_status[100];
        char  la_file_stat[100];
        char  la_stat[1024];
        char  la_status[1024];
        FILE *lp_file_stat;
        FILE *lp_file_status;
        char *lp_p1;
        char *lp_p2;
        char *lp_s;
        int   lv_len;
        int   lv_max;
        bool  lv_ret;

        lv_ret = false;

        // get boot time
        pp_s->iv_stat.iv_btime = 0;
        lp_file_stat = fopen("/proc/stat", "r");
        if (lp_file_stat != NULL) {
            for (;;) {
                lp_s = fgets(la_stat, sizeof(la_stat), lp_file_stat);
                if (lp_s == NULL)
                    break;
                if (memcmp(lp_s, "btime ", 6) == 0) {
                    lp_s = &lp_s[6];
                    sscanf(lp_s, "%llu", &pp_s->iv_stat.iv_btime);
                    break;
                }
            }
            fclose(lp_file_stat);
        }

        // get rusage
        memset(&pp_s->iv_rusage, 0, sizeof(pp_s->iv_rusage));
        getrusage(RUSAGE_SELF, &pp_s->iv_rusage);

        // get proc/<pid>/status
        sprintf(la_file_status, "/proc/%d/status", pv_pid);
        lp_file_status = fopen(la_file_status, "r");
        if (lp_file_status != NULL) {
            for (;;) {
                lp_s = fgets(la_status, sizeof(la_status), lp_file_status);
                if (lp_s == NULL)
                    break;
                if (memcmp(lp_s, "Vm ", 2) == 0) {
                    if (memcmp(lp_s, "VmData:", 7) == 0) {
                        lp_s = &lp_s[7];
                        sscanf(lp_s, "%llu %s",
                               &pp_s->iv_pidstatus.iv_vmdata,
                               pp_s->iv_pidstatus.ia_vmdata);
                    } else if (memcmp(lp_s, "VmExe:", 6) == 0) {
                        lp_s = &lp_s[6];
                        sscanf(lp_s, "%llu %s",
                               &pp_s->iv_pidstatus.iv_vmexe,
                               pp_s->iv_pidstatus.ia_vmexe);
                    } else if (memcmp(lp_s, "VmLib:", 6) == 0) {
                        lp_s = &lp_s[6];
                        sscanf(lp_s, "%llu %s",
                               &pp_s->iv_pidstatus.iv_vmlib,
                               pp_s->iv_pidstatus.ia_vmlib);
                    } else if (memcmp(lp_s, "VmPeak:", 7) == 0) {
                        lp_s = &lp_s[7];
                        sscanf(lp_s, "%llu %s",
                               &pp_s->iv_pidstatus.iv_vmpeak,
                               pp_s->iv_pidstatus.ia_vmpeak);
                    } else if (memcmp(lp_s, "VmRSS:", 6) == 0) {
                        lp_s = &lp_s[6];
                        sscanf(lp_s, "%llu %s",
                               &pp_s->iv_pidstatus.iv_vmrss,
                               pp_s->iv_pidstatus.ia_vmrss);
                    } else if (memcmp(lp_s, "VmSize:", 7) == 0) {
                        lp_s = &lp_s[7];
                        sscanf(lp_s, "%llu %s",
                               &pp_s->iv_pidstatus.iv_vmsize,
                               pp_s->iv_pidstatus.ia_vmsize);
                    } else if (memcmp(lp_s, "VmStk:", 6) == 0) {
                        lp_s = &lp_s[6];
                        sscanf(lp_s, "%llu %s",
                               &pp_s->iv_pidstatus.iv_vmstk,
                               pp_s->iv_pidstatus.ia_vmstk);
                    }
                }
            }
            fclose(lp_file_status);
        }

        // get proc/<pid>/stat
        sprintf(la_file_stat, "/proc/%d/stat", pv_pid);
        lp_file_stat = fopen(la_file_stat, "r");
        if (lp_file_stat != NULL) {
            lp_s = fgets(la_stat, sizeof(la_stat), lp_file_stat);
            pp_s->iv_pidstat.iv_pid = pv_pid;
            if (lp_s != NULL) {
                lp_p1 = strchr(lp_s, '(');
                lp_p1++;
                lp_p2 = strchr(lp_p1, ')');
                lv_len = static_cast<int>(lp_p2 - lp_p1);
                lv_max = static_cast<int>(sizeof(pp_s->iv_pidstat.ia_comm)) - 1;
                if (lv_len > lv_max)
                    lv_len = lv_max;
                memcpy(pp_s->iv_pidstat.ia_comm, lp_p1, lv_len);
                pp_s->iv_pidstat.ia_comm[lv_len] = '\0';
                lp_s = &lp_p2[2];
                // man 5 proc
                sscanf(lp_s,
                       "%c "     // state
                       "%d "     // ppid
                       "%d "     // pgrp
                       "%d "     // session
                       "%d "     // tty
                       "%d "     // tpgid
                       "%lu "    // flags
                       "%lu "    // minflt
                       "%lu "    // cminflt
                       "%lu "    // majflt
                       "%lu "    // cmajflt
                       "%llu "   // utime
                       "%llu "   // stime
                       "%llu "   // cutime
                       "%llu "   // cstime
                       "%ld "    // priority
                       "%ld "    // nice
                       "%ld "    // num_threads
                       "%ld "    // itrealvalue
                       "%llu "   // starttime
                       "%lu "    // vsize
                       "%ld ",   // rss
                       &pp_s->iv_pidstat.iv_state,       //  3
                       &pp_s->iv_pidstat.iv_ppid,        //  4
                       &pp_s->iv_pidstat.iv_pgrp,        //  5
                       &pp_s->iv_pidstat.iv_session,     //  6
                       &pp_s->iv_pidstat.iv_tty,         //  7
                       &pp_s->iv_pidstat.iv_tpgid,       //  8
                       &pp_s->iv_pidstat.iv_flags,       //  9
                       &pp_s->iv_pidstat.iv_minflt,      // 10
                       &pp_s->iv_pidstat.iv_cminflt,     // 11
                       &pp_s->iv_pidstat.iv_majflt,      // 12
                       &pp_s->iv_pidstat.iv_cmajflt,     // 13
                       &pp_s->iv_pidstat.iv_utime,       // 14
                       &pp_s->iv_pidstat.iv_stime,       // 15
                       &pp_s->iv_pidstat.iv_cutime,      // 16
                       &pp_s->iv_pidstat.iv_cstime,      // 17
                       &pp_s->iv_pidstat.iv_priority,    // 18
                       &pp_s->iv_pidstat.iv_nice,        // 19
                       &pp_s->iv_pidstat.iv_num_threads, // 20
                       &pp_s->iv_pidstat.iv_itrealvalue, // 21
                       &pp_s->iv_pidstat.iv_starttime,   // 22
                       &pp_s->iv_pidstat.iv_vsize,       // 23
                       &pp_s->iv_pidstat.iv_rss);        // 24
                lv_ret = true;
            }
            fclose(lp_file_stat);
        }

        return lv_ret;
    }

    int SB_IC_Msg_Mgr::get_pstate_user(char       *pp_rsp,
                                       const char *pp_indent) {
        char           la_buf[BUFSIZ];
        struct passwd *lp_pwbuf;
        int            lv_err;
        int            lv_len;
        struct passwd  lv_passwd;
        uid_t          lv_uid;

        lv_uid = getuid();
        lv_err = getpwuid_r(lv_uid, &lv_passwd, la_buf, BUFSIZ, &lp_pwbuf);
        if (lv_err)
            lv_len = 0;
        else {
            lv_len = sprintf(pp_rsp, "%suser id: %s, %d\n",
                             pp_indent, lv_passwd.pw_name, lv_uid);
        }

        return lv_len;
    }

    //
    // typically ms-interceptors are friends to get private access to data
    //
    int SB_IC_Msg_Mgr::prog(char *pp_rsp, int pv_rsp_len) {
        char *lp_arg;
        int   lv_arg;
        int   lv_len;

        pv_rsp_len = pv_rsp_len; // touch
        lv_len = 0;
        for (lv_arg = 0; lv_arg < gv_ic_argc; lv_arg++) {
            lp_arg = ga_ic_argv[lv_arg];
            lv_len += sprintf(&pp_rsp[lv_len], "%s:", lp_arg);
            if (memcmp(lp_arg, "call:", 5) == 0) {
                lv_len = prog_call(&lp_arg[5], pp_rsp, lv_len);
            } else if (memcmp(lp_arg, "get:", 4) == 0) {
                lv_len = prog_get(&lp_arg[4],
                                  pp_rsp,
                                  lv_len,
                                  0);
            } else if (memcmp(lp_arg, "getd:", 5) == 0) {
                lv_len = prog_get(&lp_arg[5],
                                  pp_rsp,
                                  lv_len,
                                  SQSTATE_IC_FMT_TYPE_DEC);
            } else if (memcmp(lp_arg, "geth:", 5) == 0) {
                lv_len = prog_get(&lp_arg[5],
                                  pp_rsp,
                                  lv_len,
                                  0);
            } else if ((memcmp(lp_arg, "getdh:", 6) == 0) ||
                       (memcmp(lp_arg, "gethd:", 6) == 0)) {
                lv_len = prog_get(&lp_arg[6],
                                  pp_rsp,
                                  lv_len,
                                  SQSTATE_IC_FMT_TYPE_HEX |
                                  SQSTATE_IC_FMT_TYPE_DEC);
            } else if (memcmp(lp_arg, "list\0", 5) == 0) {
                lv_len = prog_list(pp_rsp, lv_len, false);
            } else if (memcmp(lp_arg, "listd\0", 6) == 0) {
                lv_len = prog_list(pp_rsp, lv_len, true);
            } else if (memcmp(lp_arg, "set:", 4) == 0) {
                lv_len = prog_set(&lp_arg[4], pp_rsp, lv_len);
            } else if (memcmp(lp_arg, "-h", 2) == 0) {
                lv_len = 0;
                lv_len += sprintf(&pp_rsp[lv_len], "help: [ call:<fun> | get:<var> | list[d] | set:<var>=[0x]<value> ]");
                break;
            } else
                lv_len += sprintf(&pp_rsp[lv_len], "<uknown-cmd>");
            if ((lv_arg + 1) < gv_ic_argc)
                lv_len += sprintf(&pp_rsp[lv_len], " ");
        }
        return lv_len;
    }

    int SB_IC_Msg_Mgr::prog_call(char *pp_arg, char *pp_rsp, int pv_len) {
        char        la_buf[BUFSIZ];
        char       *la_libs[200];
        char       *lp_arg;
        FILE       *lp_file;
        void       *lp_handle;
        char       *lp_nl;
        const char *lp_res;
        char       *lp_s;
        void       *lp_sym;
        int         lv_inx;
        int         lv_len;
        int         lv_max_lib;

        lp_arg = pp_arg;
        lv_len = pv_len;
        lp_file = fopen("/proc/self/maps", "r");
        lv_max_lib = 0;
        lp_res = "<ok>";
        if (lp_file == NULL)
            lp_res = "<could not open /proc/self/maps>";
        else {
            // find out what libraries are loaded
            for (;;) {
                lp_s = fgets(la_buf, sizeof(la_buf), lp_file);
                if (lp_s == NULL)
                    break;
                lp_s = strchr(la_buf, '/');
                if (lp_s == NULL)
                    continue;
                lp_nl = strchr(la_buf, '\n');
                if (lp_nl != NULL)
                    *lp_nl = '\0';
                for (lv_inx = 0; lv_inx < lv_max_lib; lv_inx++) {
                    if (strcmp(lp_s, la_libs[lv_inx]) == 0)
                        break;
                }
                if (lv_inx >= lv_max_lib) {
                    la_libs[lv_inx] = strdup(lp_s);
                    lv_max_lib++;
                }
            }
            // try to open every library, try to dlsym, and call target
            lp_sym = NULL;
            for (lv_inx = 0; lv_inx < lv_max_lib; lv_inx++) {
                lp_handle = dlopen(la_libs[lv_inx], RTLD_NOW);
                if (lp_handle != NULL) {
                    lp_sym = dlsym(lp_handle, lp_arg);
                    if (lp_sym != NULL) {
                        typedef void (*Fun_Call_Proto)();
                        typedef union {
                            Fun_Call_Proto  iv_call;
                            void           *ip_call;
                        } Fun_Call_Union;
                        Fun_Call_Union lv_call;
                        lv_call.ip_call = lp_sym;
                        lv_call.iv_call();
                        dlclose(lp_handle);
                        break;
                    }
                    dlclose(lp_handle);
                }
            }
            if (lp_sym == NULL)
                lp_res = "<could not dlsym function>";
            for (lv_inx = 0; lv_inx < lv_max_lib; lv_inx++)
                free(la_libs[lv_inx]);
            fclose(lp_file);
        }
        lv_len += sprintf(&pp_rsp[lv_len], "%s=%s", lp_arg, lp_res);
        return lv_len;
    }

    int SB_IC_Msg_Mgr::prog_get(char *pp_arg,
                                char *pp_rsp,
                                int   pv_len,
                                int   pv_fmt_type) {
        char       *lp_arg;
        const char *lp_struct_str;
        void       *lp_var_addr;
        int         lv_len;
        int         lv_var_flags;
        long        lv_var_size;

        lp_arg = pp_arg;
        lv_len = pv_len;
        sb_ic_get_var(lp_arg,
                      &lp_struct_str,
                      &lp_var_addr,
                      &lv_var_size,
                      &lv_var_flags);
        if (lp_struct_str == NULL) {
            if (lv_var_size == 0)
                lv_len += sprintf(&pp_rsp[lv_len], "<no-var>");
            else
                lv_len += sqstateic_struct_format_field(lp_var_addr,
                                                        lv_var_size,
                                                        0,
                                                        pv_fmt_type,
                                                        &pp_rsp[lv_len]);
        } else {
            lv_len += sqstateic_struct_format(lp_struct_str,
                                              lp_arg,
                                              lp_var_addr,
                                              pv_fmt_type,
                                              &pp_rsp[lv_len]);
        }
        return lv_len;
    }

    int SB_IC_Msg_Mgr::prog_list(char *pp_rsp, int pv_len, bool pv_detail) {
        char            la_flags[10];
        SB_Smap_Enum   *lp_enum;
        Sqstate_Ic_Var *lp_entry;
        const char     *lp_struct_str;
        void           *lp_var_addr;
        char           *lp_var_str;
        int             lv_len;
        bool            lv_prev;
        int             lv_var_flags;
        long            lv_var_size;

        lv_len = pv_len;
        // load SB vars (side-effect)
        sb_ic_get_var(const_cast<char *>(""),
                      &lp_struct_str,
                      &lp_var_addr,
                      &lv_var_size,
                      &lv_var_flags);

        lp_enum = gp_ic_var_map->keys();
        lv_prev = false;
        while (lp_enum->more()) {
            lp_var_str = lp_enum->next();
            if (lv_prev)
                lv_len += sprintf(&pp_rsp[lv_len], "%s", ",");
            if (pv_detail) {
                lp_entry = sqstateic_var_lookup(lp_var_str);
                strcpy(la_flags, "  ");
                if (lp_entry->var_flags & SQSTATE_IC_VAR_FLAGS_ACC_R)
                    la_flags[0] = 'r';
                if (lp_entry->var_flags & SQSTATE_IC_VAR_FLAGS_ACC_W)
                    la_flags[1] = 'w';
                if (lp_entry->var_flags & SQSTATE_IC_VAR_FLAGS_TYPE_DYN)
                    strcpy(&la_flags[2], "-dyn");
                lv_len += sprintf(&pp_rsp[lv_len],
                                  "%s(%p,%ld,0x%x[%s])",
                                  lp_var_str,
                                  lp_entry->var_addr,
                                  lp_entry->var_size,
                                  lp_entry->var_flags,
                                  la_flags);
            } else
                lv_len += sprintf(&pp_rsp[lv_len], "%s", lp_var_str);
            lv_prev = true;
        }
        return lv_len;
    }

    int SB_IC_Msg_Mgr::prog_set(char *pp_arg, char *pp_rsp, int pv_len) {
        char        la_buf[1024];
        char       *lp_arg;
        char       *lp_equal;
        const char *lp_struct_str;
        char       *lp_value_str;
        void       *lp_var_addr;
        char        lv_c0;
        char        lv_c1;
        long        lv_inx;
        int         lv_len;
        int         lv_num;
        size_t      lv_str_len;
        size_t      lv_str_len_div2;
        int         lv_value;
        int64_t     lv_value64;
        int         lv_var_flags;
        long        lv_var_size;

        lv_len = pv_len;
        lp_arg = pp_arg;
        lp_equal = strchr(lp_arg, '=');
        if (lp_equal == NULL)
            lv_len += sprintf(&pp_rsp[lv_len], "<no-value>");
        else {
            *lp_equal = '\0';
            lp_value_str = &lp_equal[1];
            sb_ic_get_var(lp_arg,
                          &lp_struct_str,
                          &lp_var_addr,
                          &lv_var_size,
                          &lv_var_flags);
            if (lv_var_size == 0)
                lv_len += sprintf(&pp_rsp[lv_len], "<no-var>");
            else if (!(lv_var_flags & SQSTATE_IC_VAR_FLAGS_ACC_W)) {
                lv_len += sprintf(&pp_rsp[lv_len], "<no-write-acc>");
                lv_var_size = 0;
            }
            switch (lv_var_size) {
            case 0:
                // no var
                break;
            case 1:
            case 2:
            case 4:
                lv_len += sprintf(&pp_rsp[lv_len], "<ok>");
                if ((lp_value_str[0] == '0') &&
                    (lp_value_str[1] == 'x'))
                    sscanf(lp_value_str,
                           "%x",
                           reinterpret_cast<unsigned int *>(&lv_value));
                else
                    sscanf(lp_value_str, "%d", &lv_value);
                memcpy(lp_var_addr, &lv_value, lv_var_size);
                break;
            case 8:
                lv_len += sprintf(&pp_rsp[lv_len], "<ok>");
                if ((lp_value_str[0] == '0') &&
                    (lp_value_str[1] == 'x'))
                    sscanf(lp_value_str,
                           XXPF64X,
                           reinterpret_cast<uint64_t *>(&lv_value64));
                else
                    sscanf(lp_value_str, XXPF64, &lv_value64);
                memcpy(lp_var_addr, &lv_value64, lv_var_size);
                break;
            default:
                if ((lp_value_str[0] == '0') &&
                    (lp_value_str[1] == 'x')) {
                    lp_value_str = &lp_value_str[2];
                    lv_str_len = strlen(lp_value_str);
                    lv_str_len_div2 = lv_str_len / 2;
                    if (lv_str_len & 1)
                        lv_len += sprintf(&pp_rsp[lv_len], "<odd-num-digits>");
                    else if (lv_str_len_div2 > sizeof(la_buf))
                        lv_len += sprintf(&pp_rsp[lv_len], "<digit-limit-reached>");
                    else {
                        if (static_cast<long>(lv_str_len_div2) != lv_var_size)
                            lv_len += sprintf(&pp_rsp[lv_len], "<size-mismatch-tgt-size=%ld-set-size=" PFSZ ">",
                                              lv_var_size,
                                              lv_str_len_div2);
                        else {
                            for (lv_inx = 0; lv_inx < lv_var_size; lv_inx++) {
                                lv_c0 = lp_value_str[0];
                                lv_c1 = lp_value_str[1];
                                if ((lv_c0 >= '0') && (lv_c0 <= '9'))
                                    lv_num = lv_c0 - '0';
                                else if ((lv_c0 >= 'a') && (lv_c0 <= 'f'))
                                    lv_num = lv_c0 - 'a' + 10;
                                else if ((lv_c0 >= 'A') && (lv_c0 <= 'F'))
                                    lv_num = lv_c0 - 'A' + 10;
                                else {
                                    lv_len += sprintf(&pp_rsp[lv_len], "<invalid-digit=%c>", lv_c0);
                                    break;
                                }
                                lv_num *= 16;
                                if ((lv_c1 >= '0') && (lv_c1 <= '9'))
                                    lv_num |= lv_c1 - '0';
                                else if ((lv_c1 >= 'a') && (lv_c1 <= 'f'))
                                    lv_num |= lv_c1 - 'a' + 10;
                                else if ((lv_c1 >= 'A') && (lv_c1 <= 'F'))
                                    lv_num |= lv_c1 - 'A' + 10;
                                else {
                                    lv_len += sprintf(&pp_rsp[lv_len], "<invalid-digit=%c>", lv_c1);
                                    break;
                                }
                                la_buf[lv_inx] = static_cast<char>(lv_num);
                                lp_value_str = &lp_value_str[2];
                            }
                            if (lv_inx == lv_var_size) {
                                memcpy(lp_var_addr, la_buf, lv_var_size);
                                lv_len += sprintf(&pp_rsp[lv_len], "<ok>");
                            }
                        }
                    }
                } else
                    lv_len += sprintf(&pp_rsp[lv_len], "<must-start-with-0x>");
                break;
            }
        }
        return lv_len;
    }

    void SB_IC_Msg_Mgr::resume_threads() {
        int lv_err;

        lv_err = thread_resume_suspended();
        SB_util_assert_ieq(lv_err, 0);
    }


    void SB_IC_Msg_Mgr::suspend_threads() {
        int lv_err;

        lv_err = thread_suspend_all();
        SB_util_assert_ieq(lv_err, 0);
    }
}

void sb_ic_call_test() {
    printf("sb_ic_call_test\n");
}

//
// ms-interceptor entry point
//
void sb_ic_get_mds(BMS_SRE *pp_sre) {
    char la_rsp[MAX_RSP];
    int  lv_rsp_len;

    if (sqstateic_get_ic_args(pp_sre,
                              &gv_ic_argc,
                              ga_ic_argv,
                              MAX_IC_ARGS,
                              la_rsp,
                              MAX_RSP,
                              &lv_rsp_len)) {
        lv_rsp_len = SB_Trans::SB_IC_Msg_Mgr::get_mds(la_rsp, MAX_RSP);
    }
    sqstateic_reply(pp_sre, la_rsp, lv_rsp_len);
}

//
// ms-interceptor entry point
//
void sb_ic_get_metrics(BMS_SRE *pp_sre) {
    char la_rsp[MAX_RSP];
    int  lv_rsp_len;

    if (sqstateic_get_ic_args(pp_sre,
                              &gv_ic_argc,
                              ga_ic_argv,
                              MAX_IC_ARGS,
                              la_rsp,
                              MAX_RSP,
                              &lv_rsp_len)) {
        lv_rsp_len = SB_Trans::SB_IC_Msg_Mgr::get_metrics(la_rsp, MAX_RSP);
    }
    sqstateic_reply(pp_sre, la_rsp, lv_rsp_len);
}

//
// ms-interceptor entry point
//
void sb_ic_get_openers(BMS_SRE *pp_sre) {
    char la_rsp[MAX_RSP];
    int  lv_rsp_len;

    lv_rsp_len = SB_Trans::SB_IC_Msg_Mgr::get_openers(la_rsp, true);
    BMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl
                0,                       // replyctrlsize
                la_rsp,                  // replydata
                lv_rsp_len,              // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle
}

//
// ms-interceptor entry point
//
void sb_ic_get_openers_bin(BMS_SRE *pp_sre) {
    char la_rsp[MAX_RSP];
    int  lv_rsp_len;

    lv_rsp_len = SB_Trans::SB_IC_Msg_Mgr::get_openers(la_rsp, false);
    BMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl
                0,                       // replyctrlsize
                la_rsp,                  // replydata
                lv_rsp_len,              // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle
}

//
// ms-interceptor entry point
//
void sb_ic_get_opens(BMS_SRE *pp_sre) {
    char la_rsp[MAX_RSP];
    int  lv_rsp_len;

    lv_rsp_len = SB_Trans::SB_IC_Msg_Mgr::get_opens(la_rsp, true);
    BMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl
                0,                       // replyctrlsize
                la_rsp,                  // replydata
                lv_rsp_len,              // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle
}

//
// ms-interceptor entry point
//
void sb_ic_get_opens_bin(BMS_SRE *pp_sre) {
    char la_rsp[MAX_RSP];
    int  lv_rsp_len;

    lv_rsp_len = SB_Trans::SB_IC_Msg_Mgr::get_opens(la_rsp, false);
    BMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl
                0,                       // replyctrlsize
                la_rsp,                  // replydata
                lv_rsp_len,              // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle
}

//
// ms-interceptor entry point
//
void sb_ic_get_pstate(BMS_SRE *pp_sre) {
    char  la_rsp[MAX_RSP];
    int   lv_rsp_len;

    if (sqstateic_get_ic_args(pp_sre,
                              &gv_ic_argc,
                              ga_ic_argv,
                              MAX_IC_ARGS,
                              la_rsp,
                              MAX_RSP,
                              &lv_rsp_len)) {
        lv_rsp_len = SB_Trans::SB_IC_Msg_Mgr::get_pstate(la_rsp, MAX_RSP);
    }
    sqstateic_reply(pp_sre, la_rsp, lv_rsp_len);
}

//
// ms-interceptor entry point
//
void sb_ic_prog(BMS_SRE *pp_sre) {
    char la_rsp[MAX_RSP];
    int  lv_rsp_len;

    if (sqstateic_get_ic_args(pp_sre,
                              &gv_ic_argc,
                              ga_ic_argv,
                              MAX_IC_ARGS,
                              la_rsp,
                              MAX_RSP,
                              &lv_rsp_len)) {
        lv_rsp_len = SB_Trans::SB_IC_Msg_Mgr::prog(la_rsp, MAX_RSP);
    }
    sqstateic_reply(pp_sre, la_rsp, lv_rsp_len);
}

//
// ICs can call this to format a transid.
//
SB_Export bool sqstateic_format_transid(SB_Transid_Type *pp_transid_in,
                                        char            *pp_transid_out,
                                        int              pv_transid_out_len) {
    bool lv_ret;

    lv_ret = ms_dialect_format_transid(pp_transid_in,
                                       pp_transid_out,
                                       pv_transid_out_len);
    return lv_ret;
}

//
// ICs can call this to transid from ctrl.
// Returns non-NULL if there's a transid.
//
SB_Export SB_Transid_Type *sqstateic_get_transid(void *pp_ctrl,
                                                 int   pv_ctrl_len) {
    SB_Transid_Type *lp_transid;

    pp_ctrl = pp_ctrl;
    pv_ctrl_len = pv_ctrl_len;
    lp_transid = NULL;
    return lp_transid;
}

//
// ICs can call this to strcpy with length.
//
SB_Export int sqstateic_len_strcpy(char *pp_dest, const char *pp_src) {
    int lv_len = 0;

    while (*pp_src) {
        *pp_dest = *pp_src;
        pp_dest++;
        pp_src++;
        lv_len++;
    }
    *pp_dest = *pp_src;
    return lv_len;
}

//
// ICs can call this to set the rsp to list of options
//
SB_Export int sqstateic_set_ic_args_options(int                      pv_ic_argc,
                                            char                    *pa_ic_argv[],
                                            char                    *pp_rsp,
                                            int                      pv_rsp_len,
                                            Sqstate_Ic_Arg_Opt_Type *pp_options) {
    int  lv_arg;
    int  lv_opt;
    bool lv_options;
    int  lv_len;

    pv_rsp_len = pv_rsp_len; // touch
    lv_len = 0;
    lv_options = false;
    for (lv_arg = 0; lv_arg < pv_ic_argc; lv_arg++) {
        if ((strcmp(pa_ic_argv[lv_arg], "-h") == 0) ||
            (strcmp(pa_ic_argv[lv_arg], "-help") == 0)) {
            lv_options = true;
            break;
        }
    }

    if (lv_options) {
        lv_len += sprintf(pp_rsp, "-icarg options are { ");
        for (lv_opt = 0; pp_options[lv_opt].option_str != NULL; lv_opt++) {
            if (lv_opt != 0)
                lv_len += sprintf(&pp_rsp[lv_len], ", ");
            lv_len += sprintf(&pp_rsp[lv_len], pp_options[lv_opt].option_str);
        }
        lv_len += sprintf(&pp_rsp[lv_len], " }\n");
    }

    return lv_len;
}

//
// ICs can call this to add struct.
//
SB_Export int sqstateic_struct_add(const char            *pp_struct_name,
                                   Sqstate_Ic_Struct_Fmt  pv_formatter,
                                   void                  *pp_struct_desc) {
    Sqstate_Ic_Format_Desc *lp_fmt_desc;

    if (pp_struct_name == NULL) {
        printf("name cannot be null\n");
        abort();
    }

    if (pp_struct_desc == NULL) {
        printf("desc cannot be null\n");
        abort();
    }

    if (gp_ic_struct_map == NULL)
        gp_ic_struct_map = new SB_Smap("ic_struct_map");

    lp_fmt_desc = new Sqstate_Ic_Format_Desc();
    lp_fmt_desc->iv_struct_formatter = pv_formatter;
    lp_fmt_desc->ip_struct_desc = pp_struct_desc;
    gp_ic_struct_map->putv(pp_struct_name, lp_fmt_desc);
    return 0;
}

//
// ICs can call this to format a struct.
//
SB_Export int sqstateic_struct_format(const char *pp_struct_name,
                                      const char *pp_data_name,
                                      void       *pp_data,
                                      int         pv_fmt_type,
                                      char       *pp_rsp) {
    Sqstate_Ic_Format_Desc *lp_fmt_desc;
    int                     lv_len;

    if (gp_ic_struct_map == NULL)
        gp_ic_struct_map = new SB_Smap("ic_struct_map");
    lp_fmt_desc =
      reinterpret_cast<Sqstate_Ic_Format_Desc *>(gp_ic_struct_map->getv(pp_struct_name));
    if (lp_fmt_desc == NULL) {
        lv_len = sprintf(pp_rsp, "<could not find formatter for struct=%s>", pp_struct_name);
    } else
        lv_len = lp_fmt_desc->iv_struct_formatter(pp_data_name,
                                                  pp_data,
                                                  pp_struct_name,
                                                  lp_fmt_desc->ip_struct_desc,
                                                  pv_fmt_type,
                                                  pp_rsp);
    return lv_len;
}

//
// ICs can call this to format a struct field.
//
SB_Export int sqstateic_struct_format_field(void *pp_data,
                                            long  pv_data_size,
                                            long  pv_data_off,
                                            int   pv_fmt_type,
                                            char *pp_rsp) {
    const char *HEXDIG = "0123456789abcdef";
    char        la_hex[3];
    char       *lp_data;
    int         lv_c;
    int         lv_data4;
    long long   lv_data8;
    bool        lv_fmt_both;
    bool        lv_fmt_dec;
    bool        lv_fmt_hex;
    long        lv_inx;
    int         lv_len;

    lv_len = 0;
    if (pv_fmt_type == 0)
        pv_fmt_type = SQSTATE_IC_FMT_TYPE_HEX; // default
    lv_fmt_dec = (pv_fmt_type & SQSTATE_IC_FMT_TYPE_DEC);
    lv_fmt_hex = (pv_fmt_type & SQSTATE_IC_FMT_TYPE_HEX);
    lv_fmt_both = lv_fmt_hex & lv_fmt_dec;
    lp_data = static_cast<char *>(pp_data);
    if (lv_fmt_hex) {
        lv_len += sprintf(&pp_rsp[lv_len], "0x");
        switch (pv_data_size) {
        case 1:
            lv_data4 = lp_data[pv_data_off] & 0xff;
            lv_data4 &= 0xff;
            lv_len += sprintf(&pp_rsp[lv_len], "%02x", lv_data4);
            break;
        case 2:
            lv_data4 = *reinterpret_cast<short *>(&lp_data[pv_data_off]);
            lv_data4 &= 0xffff;
            lv_len += sprintf(&pp_rsp[lv_len], "%04x", lv_data4);
            break;
        case 4:
            lv_data4 = *reinterpret_cast<int *>(&lp_data[pv_data_off]);
            lv_len += sprintf(&pp_rsp[lv_len], "%08x", lv_data4);
            break;
        case 8:
            lv_data8 = *reinterpret_cast<long long *>(&lp_data[pv_data_off]);
            lv_len += sprintf(&pp_rsp[lv_len], "%016llx", lv_data8);
            break;
        default:
            la_hex[2] = 0;
            for (lv_inx = 0; lv_inx < pv_data_size; lv_inx++) {
                lv_c = lp_data[pv_data_off + lv_inx];
                la_hex[0] = (char) HEXDIG[lv_c >> 4];
                la_hex[1] = (char) HEXDIG[lv_c & 0xf];
                lv_len += sprintf(&pp_rsp[lv_len], la_hex);
            }
        }
        if (lv_fmt_both)
            lv_len += sprintf(&pp_rsp[lv_len], "/");
    }
    if (lv_fmt_dec) {
        switch (pv_data_size) {
        case 1:
            lv_data4 = lp_data[pv_data_off];
            lv_data4 &= 0xff;
            lv_len += sprintf(&pp_rsp[lv_len], "%d", lv_data4);
            break;
        case 2:
            lv_data4 = *reinterpret_cast<short *>(&lp_data[pv_data_off]);
            lv_data4 &= 0xffff;
            lv_len += sprintf(&pp_rsp[lv_len], "%d", lv_data4);
            break;
        case 4:
            lv_data4 = *reinterpret_cast<int *>(&lp_data[pv_data_off]);
            lv_len += sprintf(&pp_rsp[lv_len], "%d", lv_data4);
            break;
        case 8:
            lv_data8 = *reinterpret_cast<long long *>(&lp_data[pv_data_off]);
            lv_len += sprintf(&pp_rsp[lv_len], "%lld", lv_data8);
            break;
        default:
            for (lv_inx = 0; lv_inx < pv_data_size; lv_inx++) {
                lv_c = lp_data[pv_data_off + lv_inx];
                la_hex[0] = (char) HEXDIG[lv_c >> 4];
                la_hex[1] = (char) HEXDIG[lv_c & 0xf];
                lv_len += sprintf(&pp_rsp[lv_len], la_hex);
            }
        }
    }
    return lv_len;
}

//
// ICs can call this to add variables.
//
SB_Export int sqstateic_var_add(Sqstate_Ic_Var *pp_vars,
                                long            pv_sizeof_vars) {
    Sqstate_Ic_Var *lp_entry;
    size_t          lv_inx;
    size_t          lv_last;

    if (pp_vars == NULL) {
        printf("vars cannot be null\n");
        abort();
    }
    if (pv_sizeof_vars < (static_cast<long>(sizeof(Sqstate_Ic_Var)) * 2)) {
        printf("vars too small - size must be at least " PFSZ " - may miss BEGIN/END\n",
               sizeof(Sqstate_Ic_Var));
        abort();
    }
    sqstateic_var_add_check(&pp_vars[0],
                            0,
                            "begin-",
                            NULL,
                            "SQSTATE_IC_VAR_TABLE_BEGIN");
    lv_last = (pv_sizeof_vars / sizeof(Sqstate_Ic_Var)) - 1;
    sqstateic_var_add_check(&pp_vars[lv_last],
                            static_cast<int>(lv_last),
                            "end-",
                            &pp_vars[0].var_str[6],
                            "SQSTATE_IC_VAR_TABLE_END");

    if (gp_ic_var_map == NULL)
        gp_ic_var_map = new SB_Smap("ic_var_map");

    for (lv_inx = 1; lv_inx < lv_last; lv_inx++) {
        lp_entry = &pp_vars[lv_inx];
        gp_ic_var_map->putv(lp_entry->var_str, lp_entry);
    }
    return 0;
}

static void sqstateic_var_add_check(Sqstate_Ic_Var *pp_entry,
                                    int             pv_inx,
                                    const char     *pp_str,
                                    const char     *pp_var,
                                    const char     *pp_table) {
    const char *lp_s;

    if (pp_entry->var_str == NULL) {
        printf("vars[%d].var_str is NULL - may miss '%s'\n",
               pv_inx,
               pp_table);
        abort();
    }
    if (memcmp(pp_entry->var_str, pp_str, strlen(pp_str)) != 0) {
        printf("vars[%d].var_str is '%s' - expecting '%s' - may miss '%s'\n",
               pv_inx,
               pp_entry->var_str,
               pp_str,
               pp_str);
        abort();
    }
    if (pp_var != NULL) {
        lp_s = &pp_entry->var_str[strlen(pp_str)];
        if (strcmp(lp_s, pp_var) != 0) {
            printf("vars[%d].var_str is '%s' - expecting '%s%s' - may miss '%s'\n",
                   pv_inx, pp_entry->var_str, pp_str, pp_var, pp_table);
            abort();
        }
    }
    if (pp_entry->var_addr != NULL) {
        printf("vars[%d].var_addr is NOT NULL - may miss '%s'\n",
               pv_inx, pp_table);
        abort();
    }
    if (pp_entry->var_size != sizeof(Sqstate_Ic_Var)) {
        printf("vars[%d].var_size is %ld - expecting '" PFSZ "' - may miss '%s'\n",
               pv_inx,
               pp_entry->var_size,
               sizeof(Sqstate_Ic_Var),
               pp_table);
        abort();
    }
}

SB_Export int sqstateic_var_dyn_add(const char *pp_var_str,
                                    void       *pp_var_addr,
                                    long        pv_var_size,
                                    bool        pv_var_ro) {
    Sqstate_Ic_Var *lp_entry;

    if (gp_ic_var_map == NULL)
        gp_ic_var_map = new SB_Smap("ic_var_map");

    if (pp_var_str == NULL) {
        printf("var_str cannot be null\n");
        abort();
    }

    lp_entry = new Sqstate_Ic_Var;
    lp_entry->var_str = pp_var_str;
    lp_entry->var_addr = pp_var_addr;
    lp_entry->var_size = pv_var_size;
    lp_entry->var_flags = SQSTATE_IC_VAR_FLAGS_TYPE_DYN;
    if (pv_var_ro)
        lp_entry->var_flags |= SQSTATE_IC_VAR_FLAGS_ACC_R;
    else
        lp_entry->var_flags |= SQSTATE_IC_VAR_FLAGS_ACC_RW;
    gp_ic_var_map->putv(lp_entry->var_str, lp_entry);

    return 0;
}

SB_Export int sqstateic_var_dyn_del(const char *pp_var_str) {
    Sqstate_Ic_Var *lp_entry;

    if (gp_ic_var_map == NULL)
        gp_ic_var_map = new SB_Smap("ic_var_map");

    lp_entry =
      reinterpret_cast<Sqstate_Ic_Var *>(gp_ic_var_map->removev(pp_var_str));
    if (lp_entry != NULL) {
        if (lp_entry->var_flags & SQSTATE_IC_VAR_FLAGS_TYPE_DYN) {
        } else
            delete lp_entry;
    }

    return 0;
}

SB_Export Sqstate_Ic_Var *sqstateic_var_lookup(const char *pp_var_str) {
    Sqstate_Ic_Var *lp_entry;

    lp_entry =
      reinterpret_cast<Sqstate_Ic_Var *>(gp_ic_var_map->getv(pp_var_str));
    return lp_entry;
}

