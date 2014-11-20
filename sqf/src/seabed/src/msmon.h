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

#ifndef __SB_MSMON_H_
#define __SB_MSMON_H_

#include "msi.h"

extern int gv_ms_enable_messages;

extern int  ms_od_cleanup_enable();
extern bool msg_mon_accept_sock_cbt(void *pp_sock_stream);
extern int  msg_mon_close_process_com(SB_Phandle_Type *pp_phandle,
                                      bool             pv_free_oid);
extern int  msg_mon_close_process_oid(int  pv_oid,
                                      bool pv_free_oid,
                                      bool pv_send_mon);
extern void msg_mon_delete_tag(int pv_tag);
extern int  msg_mon_get_ref_count(SB_Phandle_Type *pp_phandle);
extern void msg_mon_helper_cbt(MS_Md_Type *pp_md);
extern void msg_mon_init();
extern int  msg_mon_init_attach(const char *pp_where,
                                char       *pp_name);
extern int  msg_mon_init_process_args(const char *pp_where,
                                      int        *pp_argc,
                                      char     ***pppp_argv);
extern bool msg_mon_is_self(SB_Phandle_Type *pp_phandle);
extern int  msg_mon_open_process_fs(char            *pp_name,
                                    SB_Phandle_Type *pp_phandle,
                                    int             *pp_oid);
extern void msg_mon_recv_msg(MS_Md_Type *pp_md);

#endif // !__SB_MSMON_H_
