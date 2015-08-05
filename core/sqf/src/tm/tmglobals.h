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

#ifndef TMGLOBALS_H_
#define TMGLOBALS_H_

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "tmtimer.h"


//#ifdef XATM_LIB
// XATM Library definitions
#include "xatmglob.h"
#include "xatmlib.h"

XATM_TraceMask gv_XATM_traceMask;  // XA TM tracing Mask.  0 = no tracing (default)
timeval gv_startTime;

//#else
// TM definitions
#include "tmglob.h"
//timeval gv_startTime;
int32    gv_tm_trace_level;
int32    gv_pause_state;
TX_PAUSE_STATE_TYPE gv_pause_state_type;

#ifdef DEBUG_MODE
int gv_num_retries = 0;
#endif

//#endif //XATM_LIB

#endif //TMGLOBALS_H_
