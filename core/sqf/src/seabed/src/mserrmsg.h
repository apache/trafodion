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

#ifndef __SB_MSERRMSG_H_
#define __SB_MSERRMSG_H_

#include "msi.h"

extern void  ms_err_break_err(const char *pp_where,
                              short       pv_fserr,
                              MS_Md_Type *pp_md,
                              short      *pp_results);
extern short ms_err_rtn(short pv_fserr);
extern short ms_err_rtn_fatal(const char *pp_msg,
                              short       pv_fserr);
extern short ms_err_rtn_msg(const char *pp_where,
                            const char *pp_msg,
                            short       pv_fserr);
extern short ms_err_rtn_msg_fatal(const char *pp_where,
                                  const char *pp_msg,
                                  short       pv_fserr,
                                  bool        pv_stderr);
extern short ms_err_rtn_msg_noassert(const char *pp_where,
                                     const char *pp_msg,
                                     short       pv_fserr);
extern short ms_err_rtn_noassert(short pv_fserr);
extern short ms_err_sock_rtn_msg_fatal(const char *pp_where,
                                       const char *pp_msg,
                                       int         pv_err);

#endif // !__SB_MSERRMSG_H_
