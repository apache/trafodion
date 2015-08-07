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

#ifndef XATMGLOB_H_
#define XATMGLOB_H_

#include "seabed/trace.h"
#define XATM_LIB


// Globals

enum XATM_TraceMask
{
   XATM_TraceOff        = 0x0,
   XATM_TraceError      = 0x1,   // Error conditions
   XATM_TraceXAAPI      = 0x2,   // XA API entry's & exits
   XATM_TraceExit       = 0x4,   // Procedure exits
   XATM_TraceDetail     = 0x8,   // Detail trace records
   XATM_TraceLock       = 0x10,  // lock & unlocks
   XATM_TraceRMMsg      = 0x20,  // RM Message objects
   XATM_TraceSpecial    = 0x100, // For things not normally traced
   XATM_TraceTimer      = 0x200, // For timer events only

   XATM_TraceAPIError       = XATM_TraceError | XATM_TraceXAAPI, 
   XATM_TraceExitError      = XATM_TraceError | XATM_TraceExit, 
   XATM_TraceAPIExitError   = XATM_TraceError | XATM_TraceXAAPI | XATM_TraceExit,
   XATM_TraceAPIExit        = XATM_TraceXAAPI | XATM_TraceExit,
   XATM_TraceTimerExit      = XATM_TraceTimer | XATM_TraceExit,
   XATM_TraceTimerExitError = XATM_TraceTimer | XATM_TraceExit | XATM_TraceError,

   XATM_TraceAll         = 0xffffffff
};

extern XATM_TraceMask gv_XATM_traceMask;  // XA TM tracing Mask.  0 = no tracing (default)

#define XATrace(mask, a) \
      if (mask & gv_XATM_traceMask) \
         trace_printf a

extern timeval gv_startTime;
extern class TM_Info gv_tm_info;

#endif //XATMGLOB_H_
