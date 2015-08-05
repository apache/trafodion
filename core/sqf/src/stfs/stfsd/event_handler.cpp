///////////////////////////////////////////////////////////////////////////////
// 
///  \file    event_handler.cpp
///  \brief   STFS Daemon Code to handle events
///
/// This file contains code to handle events generated in the system
/// such as Process Death, Node Up/Down, Disk Up/Down, etc.
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
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#include "stfs/stfslib.h"
#include "seabed/trace.h"

#include "stfs_metadata.h"
#include "stfs_defs.h"
#include "stfs_util.h"
#include "stfsd.h"
#include "stfs_message.h"

using namespace STFS;

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_ProcessDeathHandler
///
/// \brief  Handles the death of a process to take care of orphaned STFS 
///         files (if any)
///
/// \param  int process_node_ID
/// \param  int process_PID
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_ProcessDeathHandler(int lv_ProcessNID,
			  int lv_ProcessPID)
{
  const char       *WHERE = "STFSd_ProcessDeathHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);
  
  int lv_Status = 0;

  TRACE_PRINTF4(1, 
		"%s: Handle the death of process: %d,%d\n",
		WHERE,
		lv_ProcessNID,
		lv_ProcessPID);
		
  STFS_ExternalFileOpenerContainer *lp_EfoContainer = STFS_ExternalFileOpenerContainer::GetInstance();
  if (!lp_EfoContainer) {
    TRACE_PRINTF2(1,"%s\n", "Null EFO Container");
    return -1;
  }
  
  STFS_ExternalFileOpener lv_Efo;
  lv_Efo.sqOpenerNodeId_ = lv_ProcessNID;
  lv_Efo.sqOpenerPID_ = lv_ProcessPID;
  bool lv_Done = false;

  int lv_Index = 0;
  int lv_Stat = 0;
  while (!lv_Done) {
    STFS_OpenIdentifier* lp_OpenId = lp_EfoContainer->Get(&lv_Efo,
							  lv_Index);
    if (!lp_OpenId) {
      lv_Done = true;
      continue;
    }

    if (lp_OpenId) {
      TRACE_PRINTF4(3,
		    "%s: Open Id: %d,%ld\n",
		    WHERE,
		    lp_OpenId->sqOwningDaemonNodeId,
		    lp_OpenId->openIdentifier);
      lv_Stat = STFSd_close(lp_OpenId);
      if (lv_Stat != 0) {
	lv_Index++;
      }
    }
  }

  return lv_Status;

}
