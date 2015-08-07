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
//-------------------------------------------------------------------
//
//  XARM Header file
//-------------------------------------------------------------------

#ifndef XARMAPI_H_
#define XARMAPI_H_
#include <sys/time.h>

#include "dtm/xa.h"
#include "xatmglob.h"
#include "xaglob.h"




// Globals
#define XARM_RM_NAME "SEAQUEST_RM_XARM"


XATM_TraceMask gv_XATM_traceMask;  // XA TM tracing Mask.  0 = no tracing (default)
timeval gv_startTime;

int tm_xa_recover_send(int pv_rmid, int64 pv_count, int64 pv_flags,
                       int pv_index = 0, int pv_node=-1, bool pv_dead_tm=true);
int tm_xa_recover_waitReply(int *pp_rmid, XID *pp_xids, int64 *pp_count,
                            bool *pp_end, int *pp_index, int pv_rm_wait_time, int *pp_int_error);

#endif //XARMAPI_H_
