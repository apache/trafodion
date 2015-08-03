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

#ifndef TMGLOB_H_
#define TMGLOB_H_

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/trace.h"

// define DEBUG_MODE here to enable additional diagnostics
//#define DEBUG_MODE

// Globals

extern int32 gv_tm_trace_level;  // TM trace level 0 = no tracing (default)
extern timeval gv_startTime;
extern int32 gv_pause_state;

typedef enum {
   TX_PAUSE_STATE_TYPE_OFF   = 0,
   TX_PAUSE_STATE_TYPE_FIXED,
   TX_PAUSE_STATE_TYPE_RANDOM
} TX_PAUSE_STATE_TYPE;
extern TX_PAUSE_STATE_TYPE gv_pause_state_type;

#define TMTrace(level, a) \
            {if (gv_tm_trace_level >= level) \
                trace_printf a;}

//#ifdef DEBUG_MODE
// This code implements the testpoints for testing tx recovery from particular txn states.
// Note that we don't want to pause if the state is no-tx, beginning, or active.
#define TM_TEST_PAUSE(state) \
    {if (state == gv_pause_state) { \
        if (state != TM_TX_STATE_NOTX && state !=  TM_TX_STATE_BEGINNING && state != TM_TX_STATE_ACTIVE) { \
           TMTrace(1, ("TM_TEST_PAUSE State %d hit. Pausing transaction for %dusec.\n", \
                gv_pause_state, TM_TEST_PAUSE_INTERVAL)); \
           SB_Thread::Sthr::sleep(TM_TEST_PAUSE_INTERVAL); \
        }}}
#define TM_TEST_PAUSE_NEXT() \
    {if (gv_pause_state_type == TX_PAUSE_STATE_TYPE_RANDOM) { \
        gv_pause_state = rand() % TM_TX_STATE_LAST; \
    }}
//#else
//#define TM_TEST_PAUSE(state) ;
//#define TM_TEST_PAUSE_NEXT();
//#endif

#endif //TMGLOB_H_
