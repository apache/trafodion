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

#ifndef __TMREGISTRY_H
#define __TMREGISTRY_H

#include <string.h>
#include "seabed/ms.h"
#include "dtm/tm_util.h"

#define DTM_CP_INTERVAL "DTM_CP_INTERVAL"
#define DTM_STATS_INTERVAL "DTM_STATS_INTERVAL"
#define DTM_TX_ABORT_TIMEOUT "DTM_TX_ABORT_TIMEOUT"
#define DTM_THREAD_MODEL "DTM_THREAD_MODEL"
#define DTM_GLOBAL_UNIQUE_SEQ_NUM "DTM_GLOBAL_UNIQUE_SEQ_NUM"
#define DTM_RUN_MODE "DTM_RUN_MODE"
#define DTM_INCARNATION_NUM "DTM_INCARNATION_NUM"
#define DTM_NEXT_SEQNUM_BLOCK "DTM_NEXT_SEQNUM_BLOCK"
#define DTM_TM_STATS "DTM_TM_STATS"
#define DTM_TM_STATS_INTERVAL "DTM_TM_STATS_INTERVAL"
#define DTM_TM_RMRETRY_INTERVAL "DTM_TM_RMRETRY_INTERVAL"
#define DTM_TM_TMRESTARTRETRY_INTERVAL "DTM_TM_TMRESTARTRETRY_INTERVAL"
#define DTM_TM_TS_MODE "DTM_TM_TS_MODE"
#define DTM_RM_PARTIC "DTM_RM_PARTIC"
#define DTM_TLOG_PER_TM "DTM_TLOG_PER_TM"
#define DTM_RM_PARTIC_RECOV "DTM_RM_PARTIC_RECOV"
#define DTM_TM_SHUTDOWNABRUPTNOW "DTM_TM_SHUTDOWNABRUPTNOW"
#define DTM_BROADCAST_ROLLBACKS "DTM_BROADCAST_ROLLBACKS"
#define DTM_EARLYCOMMITREPLY "DTM_EARLYCOMMITREPLY"

#define DTM_MAX_NUM_TRANS "DTM_MAX_NUM_TRANS"
#define DTM_STEADYSTATE_LOW_TRANS "DTM_STEADYSTATE_LOW_TRANS"
#define DTM_STEADYSTATE_HIGH_TRANS "DTM_STEADYSTATE_HIGH_TRANS"

#define DTM_MAX_NUM_THREADS "DTM_MAX_NUM_THREADS"
#define DTM_STEADYSTATE_LOW_THREADS "DTM_STEADYSTATE_LOW_THREADS"
#define DTM_STEADYSTATE_HIGH_THREADS "DTM_STEADYSTATE_HIGH_THREADS"

#define DTM_MAX_NUM_RMMESSAGES "DTM_MAX_NUM_RMMESSAGES"
#define DTM_STEADYSTATE_LOW_RMMESSAGES "DTM_STEADYSTATE_LOW_RMMESSAGES"
#define DTM_STEADYSTATE_HIGH_RMMESSAGES "DTM_STEADYSTATE_HIGH_RMMESSAGES"

#define DTM_STALL_PHASE_2 "DTM_STALL_PHASE_2"
#define DTM_RM_WAIT_TIME "DTM_RM_WAIT_TIME"
#define DTM_TRANS_HUNG_RETRY_INTERVAL "DTM_TRANS_HUNG_RETRY_INTERVAL"
#define DTM_TIMERTHREAD_WAIT "DTM_TIMERTHREAD_WAIT"
#define DTM_TM_TRACE "DTM_TM_TRACE"
#define DTM_XATM_TRACE "DTM_XATM_TRACE"
#define DTM_AUDIT_MODE "DTM_AUDIT_MODE"
#define DTM_PERF_STATS "DTM_PERF_STATS"
#define DTM_TEST_PAUSE_STATE "DTM_TEST_PAUSE_STATE"
#define DTM_RECOVERING_TX_COUNT "DTM_RECOVERING_TX_COUNT"
#define DTM_OVERRIDE_AUDIT_INCONSISTENCY "DTM_OVERRIDE_AUDIT_INCONSISTENCY"
#define DTM_ERROR_SHUTDOWN_MODE "DTM_ERROR_SHUTDOWN_MODE"

#define CLUSTER_GROUP "CLUSTER"

#define AUDIT_MGMT_PROC "AUDIT_MGMT_PROC"

int32 tm_reg_get (MS_Mon_ConfigType   pv_type,
                  char                *pp_group,
                  char                *pp_key,
                  char                *pp_value);

int32 tm_reg_set (MS_Mon_ConfigType   pv_type,
                  char                *pp_group,
                  char                *pp_key,
                  char                *pp_value);



#endif


