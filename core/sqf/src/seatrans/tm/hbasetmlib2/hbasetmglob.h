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

#ifndef HBASETMGLOB_H_
#define HBASETMGLOB_H_

#include <sys/types.h>
#include <sys/time.h>
#include "seabed/trace.h"

// Globals

enum HBASETM_TraceMask
{
   HBASETM_TraceOff        = 0x0,
   HBASETM_TraceError      = 0x1,   // Error conditions
   HBASETM_TraceAPI        = 0x2,   // API entry's & exits
   HBASETM_TraceExit       = 0x4,   // Procedure exits
   HBASETM_TraceDetail     = 0x8,   // Detail trace records
   HBASETM_TraceLock       = 0x10,  // lock & unlocks
   HBASETM_TraceRMMsg      = 0x20,  // RM Message objects
   HBASETM_TraceSpecial    = 0x100, // For things not normally traced
   HBASETM_TraceTimer      = 0x200, // For timer events only

   HBASETM_TraceAPIError       = HBASETM_TraceError | HBASETM_TraceAPI, 
   HBASETM_TraceExitError      = HBASETM_TraceError | HBASETM_TraceExit, 
   HBASETM_TraceAPIExitError   = HBASETM_TraceError | HBASETM_TraceAPI | HBASETM_TraceExit,
   HBASETM_TraceAPIExit        = HBASETM_TraceAPI | HBASETM_TraceExit,
   HBASETM_TraceTimerExit      = HBASETM_TraceTimer | HBASETM_TraceExit,
   HBASETM_TraceTimerExitError = HBASETM_TraceTimer | HBASETM_TraceExit | HBASETM_TraceError,

   HBASETM_TraceAll         = 0xffffffff
};

//class CHbaseTM;
//CHbaseTM gv_HbaseTM;

#define HBASETrace(mask, a) \
      if (mask & gv_HBASETM_traceMask) \
         trace_printf a
#define HDR "HBaseTM: "

//extern timeval gv_startTime;

extern HBASETM_TraceMask gv_HBASETM_traceMask;  // HBase TM tracing Mask.  0 = no tracing (default)
//extern int tm_log_write(int, int, char*);
#endif //HBASETMGLOB_H_
