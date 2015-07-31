///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfsd_createfragment.cpp
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
#include "stfs_session.h"
#include "stfsd_openers.h"

namespace STFS {
///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_STFSd_fopeners
///
/// \brief  Processes an fopeners request
///
/// \param  stfs_fhndl_t             pv_Fhandle
/// \param  struct STFS_OpenersSet   pp_OpenersSet[]
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_fopeners(stfs_fhndl_t             pv_Fhandle,
               struct STFS_OpenersSet  *pp_OpenersSet)
{
  const char       *WHERE = "STFSd_fopeners";
  STFS_ScopeTrace   lv_st(WHERE,2);

  //Get all the opener information using the stfs_util::STFS_openers() function
  //Put the information in the structure and pass it back to caller

  int lv_Count = 0;
  int lv_OpenRet = 0;
  long *lp_PrevIndex = new long(0);

  while((lv_Count < STFS_MAX_OPENERS) && (lv_OpenRet == 0)) {

    lv_OpenRet = STFS_util::STFS_fopeners(pv_Fhandle,
                                          lp_PrevIndex,
                                          pp_OpenersSet->Nid[*lp_PrevIndex],
                                          pp_OpenersSet->Pid[*lp_PrevIndex],
                                          pp_OpenersSet->OpenerName[*lp_PrevIndex],
                                          pp_OpenersSet->Path[*lp_PrevIndex]);
    
    if(lv_OpenRet < 0) {
      delete lp_PrevIndex;
      return -1;
    }
    if(lv_OpenRet != 1) {
      lv_Count++;
    }
  }

  TRACE_PRINTF2(4,"Number of Openers found: %d\n", lv_Count);
                    
  pp_OpenersSet->Count = lv_Count;

  delete lp_PrevIndex;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_openers
///
/// \brief  Processes an openers request
///
/// \param  stfs_nodeid_t            pv_Nid
/// \param  char                    *pp_Path
/// \param  struct STFS_OpenersSet   pp_OpenersSet
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_openers(stfs_nodeid_t            pv_Nid,
              char                    *pp_Path,
              struct STFS_OpenersSet  *pp_OpenersSet)
{
  const char       *WHERE = "STFSd_openers";
  STFS_ScopeTrace   lv_st(WHERE,2);

  //Get all the opener information using the stfs_util::STFS_openers() function
  //Put the information in the structure and pass it back to caller
  
  //Now we want a while loop to get all the openers data.  Must not exceed size
  //    of the array.
  int lv_Count = 0;
  int lv_OpenRet = 0;
  long *lp_PrevIndex = new long(0);

  while((lv_Count < STFS_MAX_OPENERS) && (lv_OpenRet == 0)) {

    strcpy(pp_OpenersSet->Path[*lp_PrevIndex], pp_Path);

    lv_OpenRet = STFS_util::STFS_openers(pv_Nid,
                                         pp_OpenersSet->Path[*lp_PrevIndex],
                                         lp_PrevIndex,
                                         pp_OpenersSet->Nid[*lp_PrevIndex],
                                         pp_OpenersSet->Pid[*lp_PrevIndex],
                                         pp_OpenersSet->OpenerName[*lp_PrevIndex]);
    
    if(lv_OpenRet < 0) {
      //TODO: Error Checking
      delete lp_PrevIndex;
      return -1;
    }
    if(lv_OpenRet != 1) {
      lv_Count++;
    }
  }

  TRACE_PRINTF2(4,"Number of openers found: %d\n", lv_Count);
                    
  pp_OpenersSet->Count = lv_Count;

  delete lp_PrevIndex;

  return 0;
}

} //namespace STFS
