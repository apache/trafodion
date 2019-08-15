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

#ifndef __SB_MSX_H_
#define __SB_MSX_H_

#include "seabed/ms.h"

#include "transport.h"

#ifdef USE_SB_MALLOC
#define MS_BUF_MGR_ALLOC(len) malloc(len)
#define MS_BUF_MGR_FREE(ptr) free(ptr)
#else
#define MS_BUF_MGR_ALLOC(len) gv_ms_buf_mgr.ua.ialloc(len)
#define MS_BUF_MGR_FREE(ptr) gv_ms_buf_mgr.uf.ifree(ptr)
#endif

typedef enum {
    MSG_MON_PROCESS_SHUTDOWN_TYPE_REG,
    MSG_MON_PROCESS_SHUTDOWN_TYPE_FAST,
    MSG_MON_PROCESS_SHUTDOWN_TYPE_NOW
} Msg_Mon_Process_Shutdown_Type;

extern bool                  gv_ms_assert_chk;
extern bool                  gv_ms_assert_chk_send;
extern bool                  gv_ms_attach;
extern MS_Buf_Mgr_Type       gv_ms_buf_mgr;
extern int                   gv_ms_buf_options;
extern bool                  gv_ms_calls_ok;
extern bool                  gv_ms_client_only;
extern int                   gv_ms_conn_idle_timeout;
extern bool                  gv_ms_conn_reuse;
extern int                   gv_ms_disc_sem;
extern bool                  gv_ms_disc_sem_rob;
extern bool                  gv_ms_disc_sem_stats;
extern SB_Ms_Tl_Event_Mgr    gv_ms_event_mgr;
extern bool                  gv_ms_ic_ibv;
extern bool                  gv_ms_inited;
extern bool                  gv_ms_init_test;
extern MS_Lim_Queue_Cb_Type  gv_ms_lim_cb;
extern SB_Recv_Queue         gv_ms_lim_q;
extern SB_Ts_Lmap            gv_ms_ldone_map;
extern bool                  gv_ms_mon_calls_ok;
extern bool                  gv_ms_msg_timestamp;
extern bool                  gv_ms_process_comp;
extern SB_Recv_Queue         gv_ms_recv_q;
extern bool                  gv_ms_recv_q_proc_death;
extern bool                  gv_ms_shutdown_called;
extern bool                  gv_ms_shutdown_fast;
extern int                   gv_ms_streams;
extern bool                  gv_ms_trans_sock;

extern short                 msg_buf_read_data_int(int     pv_msgid,
                                                   char   *pp_reqdata,
                                                   int     pv_bytecount);
extern int                   msg_init_com(int    *pp_argc,
                                          char ***pppp_argv,
                                          int     pv_mpi_init,
                                          bool    pv_attach,
                                          bool    pv_forkexec,
                                          char   *pp_name,
                                          bool    pv_msg)
                                          SB_THROWS_FATAL;
extern void                  msg_init_env(int pv_argc, char **ppp_argv);
extern int                   msg_test_assert_disable();
extern void                  msg_test_assert_enable(int pv_state);
extern void                  ms_abandon_cbt(MS_Md_Type *pp_md, bool pv_add);
extern void                  ms_buf_free(void *);
extern void                 *ms_buf_malloc(size_t);
extern void                  ms_err_check_mpi_pathdown_ok(const char *pp_where,
                                                          const char *pp_msg,
                                                          short       pv_fserr);
extern short                 ms_err_mpi_rtn_msg(const char *pp_where,
                                                const char *pp_msg,
                                                int         pv_mpierr);
extern short                 ms_err_mpi_rtn_msg_fatal(const char *pp_where,
                                                      const char *pp_msg,
                                                      int         pv_mpierr);
extern short                 ms_err_mpi_rtn_msg_noassert(const char *pp_where,
                                                         const char *pp_msg,
                                                         int         pv_mpierr);
extern void                  ms_fifo_setup(int pv_orig_fd, char *pp_fifo_name, bool pv_remap_fd = true); // pv_remap_fd to monitor
extern void                  ms_free_recv_bufs(MS_Md_Type *pp_md);
extern SB_Comp_Queue        *ms_fsdone_get_comp_q(bool pv_ts);
extern void                  ms_gather_info(const char *pp_where);
extern void                  ms_getenv_bool(const char *pp_key, bool *pp_val);
extern void                  ms_getenv_int(const char *pp_key, int *pp_val);
extern void                  ms_getenv_longlong(const char *pp_key,
                                                long long  *pp_val);
extern const char           *ms_getenv_str(const char *pp_key);
extern void                  ms_ldone_cbt(MS_Md_Type *pp_md);
extern SB_Comp_Queue        *ms_ldone_get_comp_q();
extern void                  ms_msg_set_requeue(const char *pp_where,
                                                int         pv_msgid,
                                                bool        pv_requeue);
extern SB_Trans::Stream_Base*ms_od_map_oid_to_stream(int pv_oid);
extern char                 *ms_od_map_phandle_to_name(SB_Phandle_Type *pp_phandle);
extern SB_Trans::Stream_Base*ms_od_map_phandle_to_stream(SB_Phandle_Type *pp_phandle);
extern void                  ms_recv_q_proc_death(int            pv_nid,
                                                  int            pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                  SB_Verif_Type  pv_verif,
#endif
                                                  bool           pv_use_stream,
                                                  void          *pp_stream);
extern "C" const char       *ms_seabed_vers();
extern void                  ms_shutdown();
extern void                  ms_transid_clear(MS_Mon_Transid_Type  pv_transid,
                                              MS_Mon_Transseq_Type pv_startid);
extern int                   ms_transid_get(bool                  pv_supp,
                                            bool                  pv_trace,
                                            MS_Mon_Transid_Type  *pp_transid,
                                            MS_Mon_Transseq_Type *pp_startid);
extern int                   ms_transid_reg(MS_Mon_Transid_Type  pv_transid,
                                            MS_Mon_Transseq_Type pv_startid);
extern int                   ms_transid_reinstate(MS_Mon_Transid_Type  pv_transid,
                                                  MS_Mon_Transseq_Type pv_startid);
extern void                  ms_util_fill_phandle_name(SB_Phandle_Type *pp_phandle,
                                                       char            *pp_name,
                                                       int              pv_nid,
                                                       int              pv_pid
#ifdef SQ_PHANDLE_VERIFIER
                                                      ,SB_Verif_Type    pv_verif
#endif
                                                      );
extern int                   msg_mon_process_shutdown_ph1(const char                    *pp_where,
                                                          bool                           pv_finalize,
                                                          Msg_Mon_Process_Shutdown_Type  pv_shutdown_type);
extern short                 xmsg_break_com(int               pv_msgid,
                                            short            *pp_results,
                                            SB_Phandle_Type  *pp_phandle,
                                            MS_Md_Type      **ppp_md,
                                            bool              pv_reply_ctrl);
#endif // !__SB_MSX_H_
