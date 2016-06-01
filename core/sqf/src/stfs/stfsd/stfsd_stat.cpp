///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfsd_stat.cpp
/// \brief  
///   
/// This file contains the implementation of the STFSd_createFragment() 
/// function.
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
///////////////////////////////////////////////////////////////////////////////


#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <iostream>


#include "stfs/stfslib.h"

#include "stfs_metadata.h"
#include "stfs_defs.h"
#include "stfs_util.h"
#include "stfs_message.h"
#include "stfsd_stat.h"

namespace STFS {

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_stat
///
/// \brief  Processes a stat request
///
/// \param  stfs_nodeid_t            pv_Nid
/// \param  char                    *pp_Path
/// \param  Stfs_StatSet            *pp_StatSet
/// \param  stfs_statmask_t          pv_Mask
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_stat (stfs_nodeid_t            pv_Nid,
            char                    *pp_Path,
            struct STFS_StatSet     *pp_StatSet,
            stfs_statmask_t          pv_Mask)
{
  const char       *WHERE = "STFSd_stat";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Count = 0;
  int lv_OpenRet = 0;
  long *lp_PrevIndex = new long(0);

  while((lv_Count < STFS_MAX_OPENERS) && (lv_OpenRet == 0)) {

    strcpy(pp_StatSet->Path[*lp_PrevIndex], pp_Path);

    lv_OpenRet = STFS_util::STFS_stat(pv_Nid,
                                      pp_StatSet->Path[*lp_PrevIndex],
                                      lp_PrevIndex,
                                      pv_Mask,
                                     &pp_StatSet->Stat[*lp_PrevIndex]);
    
    if(lv_OpenRet < 0) {
      //TODO: Error Checking
      return -1;
    }
    if(lv_OpenRet != 1) {
      lv_Count++;
    }
  }

  TRACE_PRINTF2(4,"Number of files found by STFS_stat: %d\n", lv_Count);
  pp_StatSet->Count = lv_Count;

  return 0;
}

} //namespace STFS
