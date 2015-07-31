#ifndef STFS_SIGILL_H
#define STFS_SIGILL_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_sigill.h
///  \brief   Violent termination functions
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

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "stfs_util.h"

namespace STFS {


  inline  void STFS_SigIll (void) { 	
    //    throw STFS_SIGILL_EXCEPT;

    TRACE_PRINTF1(1, "STFS_SIGILL Invocation");
    char lv_buf[256];
    sprintf(lv_buf, "STFS Software Failure on Node: %d\n", STFS_util::GetMyNodeId());
    STFS_util::STFS_evlog(STFS_EVENT_SOFTWARE_FAILURE, // Rev: may change to something else
			  SQ_LOG_ERR,
			  (char *) &lv_buf);
    

    if (!STFS_util::executingInDaemon_) {
      abort();
    }
  }
}

#endif
