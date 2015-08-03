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

#ifndef __SB_LOGAGGR_H_
#define __SB_LOGAGGR_H_

typedef enum {
    SB_LOG_AGGR_CAP_AFIRST = 0,
    SB_LOG_AGGR_CAP_FD_TABLE_CAP_INC,
    SB_LOG_AGGR_CAP_MD_RECV_HI_CAP_INC,
    SB_LOG_AGGR_CAP_MD_SEND_HI_CAP_INC,
    SB_LOG_AGGR_CAP_MD_TABLE_CAP_INC,
    SB_LOG_AGGR_CAP_OD_TABLE_CAP_INC,
    SB_LOG_AGGR_CAP_STREAM_TABLE_CAP_INC,
    SB_LOG_AGGR_CAP_THREAD_TABLE_CAP_INC,
    SB_LOG_AGGR_CAP_ZLAST
} SB_Log_Aggr_Cap_Type;

extern void sb_log_aggr_cap_log_flush();
extern void sb_log_aggr_cap_log(const char *pp_where,
                                int         pv_cap_type,
                                int         pv_cap_size);

#endif // !__SB_LOGAGGR_H_
