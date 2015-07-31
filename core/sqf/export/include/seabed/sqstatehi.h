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

//
// sqstatehi module
//
#ifndef __SB_SQSTATEHI_H_
#define __SB_SQSTATEHI_H_

#include "seabed/sqstate.h"

//
// may be used to get ic arguments
//
SB_Export bool sqstateic_get_ic_args(BMS_SRE *sre,
                                     int     *ic_argc,
                                     char    *ic_argv[],
                                     int      ic_argc_max,
                                     char    *rsp,
                                     int      rsp_len,
                                     int     *rsp_len_ptr);

//
// may be used to do ic reply
//
SB_Export void sqstateic_reply(BMS_SRE *sre,
                               char    *rsp,
                               int      rsp_len);

//
// may be used to print ic reply
//
SB_Export void sqstatepi_print_ic_reply(char *rsp, int rsp_len);

//
// may be used print something from a pi
//
SB_Export void sqstatepi_printf(const char *format, ...)
            __attribute__((format(printf, 1, 2)));

//
// may be used to send msg to ic
//
// returns ok (call sqstatepi_print_ic_reply to print reply)
//
SB_Export bool sqstatepi_send_ic_ok(const char                  *module,
                                    const char                  *call,
                                    MS_Mon_Node_Info_Entry_Type *node,
                                    MS_Mon_Process_Info_Type    *proc,
                                    Sqstate_Pi_Info_Type        *info,
                                    const char                  *lib,
                                    char                        *rsp,
                                    int                          rsp_len,
                                    int                         *rsp_len_ptr);

//
// may be used to print OK msg
//
SB_Export void sqstatepi_verbose_ok(Sqstate_Pi_Info_Type *info,
                                    int                   fserr,
                                    const char           *msg);

#endif // !__SB_SQSTATEHI_H_
