#ifndef _STFS_DEFS_H
#define _STFS_DEFS_H
///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_defs.h
///  \brief   Some commonly used defines/types used in the STFS code internally
///    
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
///////////////////////////////////////////////////////////////////////////////
#include "stdio.h"
#include "seabed/trace.h"
#include "stfs/stfslib.h"

#ifdef DEBUG
#define ASSERT(a)  assert((a))
#else
#define ASSERT(a)
#endif

#define STFS_MAX_OPENERS     100
#define STFS_MAX_STAT        100

#define STFS_MSGBUFFER_MAX   32768

// Process Name for Openers
#define STFS_PROC_MAX        16

// (Maximum length + 1) of a fragment file's directory name
#define STFS_DIRNAME_MAX     1048

#define STFS_GARBAGE_CHAR 'S'

#ifdef DEBUG

#define DEBUG_CHECK_EYECATCHER_BASE(pointer_to_obj)     \
  if (! pointer_to_obj->IsEyeCatcherValid()) {          \
    STFS_SigIll();                                      

// first parameter : pointer to an object (derived from STFS_Root)
// second parameter: return value when the eye catcher is corrupt
#define DEBUG_CHECK_EYECATCHER(pointer_to_obj,retval)   \
  DEBUG_CHECK_EYECATCHER_BASE(pointer_to_obj)           \
    return retval;                                      \
  }

// Use when the method/function returns a void
#define DEBUG_CHECK_EYECATCHER_VOID(pointer_to_obj) \
  DEBUG_CHECK_EYECATCHER_BASE(pointer_to_obj)	    \
       return;                                      \
       }

#else
#define DEBUG_CHECK_EYECATCHER 
#endif

namespace STFS {

  struct STFS_OpenIdentifier {
    int  sqOwningDaemonNodeId;    // Owning Daemon's SQ Node ID
    long openIdentifier;          // Open ID - unique on the Daemon's Node

    bool operator==(const STFS_OpenIdentifier & other) const
    {
      if (this->sqOwningDaemonNodeId != other.sqOwningDaemonNodeId) {
	return false;
      }

      if (this->openIdentifier != other.openIdentifier) {
	return false;
      }

      return true;
    }
  };
  
  struct STFS_OpenersSet {
    char            Path[STFS_MAX_OPENERS][STFS_PATH_MAX];
    stfs_nodeid_t   Nid[STFS_MAX_OPENERS];
    pid_t           Pid[STFS_MAX_OPENERS];
    char            OpenerName[STFS_MAX_OPENERS][STFS_PROC_MAX];
    int             Count;
  };

  struct STFS_StatSet {
    char             Path[STFS_MAX_OPENERS][STFS_PATH_MAX];
    struct stfs_stat Stat[STFS_MAX_STAT];
    int              Count;
  };

  // varoffset_t is how messages index into the current buffer's variable space
  typedef int varoffset_t;

}

#define TRACE_PRINTF_BASIC(pv_TraceLevel)	\
  if (STFS_util::traceLevel_ >= pv_TraceLevel)  \
    trace_printf( 

#define TRACE_PRINTF1(pv_TraceLevel,a1)		\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1);	

#define TRACE_PRINTF2(pv_TraceLevel,a1,a2)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2);	

#define TRACE_PRINTF3(pv_TraceLevel,a1,a2,a3)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3);	

#define TRACE_PRINTF4(pv_TraceLevel,a1,a2,a3,a4)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3,a4);	

#define TRACE_PRINTF5(pv_TraceLevel,a1,a2,a3,a4,a5)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3,a4,a5);	

#define TRACE_PRINTF6(pv_TraceLevel,a1,a2,a3,a4,a5,a6)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3,a4,a5,a6);	
    
#define TRACE_PRINTF7(pv_TraceLevel,a1,a2,a3,a4,a5,a6,a7)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3,a4,a5,a6,a7);	


//  Environment Variable names for forcing fragment creation
#define FORCEFRAGCREATION 'SQ_NUM_WRITES_PER_FRAG'
#define FRAGPARTIALWRITE 'SQ_WRITE_PARTIAL_FRAG'

#endif
