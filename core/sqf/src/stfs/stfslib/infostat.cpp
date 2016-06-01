///////////////////////////////////////////////////////////////////////////////
// 
/// \file    infostat.cpp
/// \brief   STFSINFO_stat and STFSINFO_fstat implementation
///   
/// This file contains the implementation of the STFS_openers() and 
/// STFS_fopeners() function, starting with the functional STFSINFO_openers 
/// function and drilling down to supporting functions.
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
#include "stfsd.h"

#include "send.h"

namespace STFS { 

  int STFSINFO_fstat(stfs_fhndl_t      pv_Fhandle,
                     stfs_statmask_t   pv_Mask,
                     struct stfs_stat *pp_Buf)
  {
    const char     *WHERE = "STFSLIB_fstat";
    STFS_ScopeTrace lv_st(WHERE);

    int lv_Ret = 0;
    static STFS_StatSet lv_StatSet;

    STFS_ExternalFileHandle *lp_EFH = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
    if(lp_EFH == NULL)
    {
      TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileHandle\n");
      return -1;
    }

    STFS_ExternalFileMetadata *lp_EFM = lp_EFH->EfmGet();
    if(lp_EFM == NULL)
    {
      TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileMetadata\n");
      return -1;
    }
    
    char lv_FileName[STFS_NAME_MAX];
    lv_FileName[0] = '\0';

    strcpy(lv_FileName, lp_EFM->ExternalFileNameGet());
    if(lv_FileName[0] == '\0') {
      TRACE_PRINTF1(1,"Error while Getting External Filename\n");
     }
      
     if((pv_Fhandle == 0) || (pv_Mask == 0) || (!pp_Buf))
     {} 
#ifdef SQ_PACK
    lv_Ret = SendToSTFSd_stat(-1,
                              lv_FileName,
                             &lv_StatSet,
                              pv_Mask,
                              STFS_util::GetMyNodeId(),
                              STFS_util::GetMyPID());
#else
    lv_Ret = STFSd_stat (-1, 
                         lv_FileName,
                        &lv_StatSet,
                         pv_Mask);
#endif

    if(lv_Ret != 0) {
    // TODO: Error Checking
      return -1;
    }

    //Setting the output parameters
    memcpy(pp_Buf, &lv_StatSet.Stat[0], sizeof(stfs_stat));
    if(!pp_Buf) {
      //Error getting stfs_stat structure
      return -1;
    }

    return lv_Ret;
  }

  int STFSINFO_stat(stfs_nodeid_t     pv_Nid, 
                    char             *pp_Path, 
                    long             *pp_PrevIndex, 
                    stfs_statmask_t   pv_Mask, 
                    struct stfs_stat *pp_Buf)
  {
    const char     *WHERE = "STFSLIB_stat";
    STFS_ScopeTrace lv_st(WHERE);

    int lv_Ret = 0;
    static STFS_StatSet lv_StatSet;

    //static variable of a structure that holds the openers
    //static struct OpenersStruct pv_OpenersStruct;
    if((pv_Nid == 0) || (!pp_Path) || (!pp_PrevIndex) || 
       (pv_Mask== 0) || (!pp_Buf)) 
    {}

    if(*pp_PrevIndex == 0)
    {
#ifdef SQ_PACK
        lv_Ret = SendToSTFSd_stat(pv_Nid,
                                  pp_Path,
                                 &lv_StatSet,
                                  pv_Mask,
                                  STFS_util::GetMyNodeId(),
                                  STFS_util::GetMyPID());
#else
        lv_Ret = STFSd_stat(pv_Nid, 
                            pp_Path,
                           &lv_StatSet,
                            pv_Mask);
#endif

      }

    if(lv_Ret != 0) {
    // TODO: Error Checking
      return -1;
    }
    if((*pp_PrevIndex) == lv_StatSet.Count) {
      // We returned the last element already. Return 1 to signify no more openers.
      return 1;
    }
    //Setting the output parameters
    strcpy(pp_Path, lv_StatSet.Path[*pp_PrevIndex]);
    memcpy(pp_Buf, &lv_StatSet.Stat[*pp_PrevIndex], sizeof(stfs_stat));
    if(!pp_Buf) {
      //Error getting stfs_stat structure
      return -1;
    }

    *pp_PrevIndex += 1;
    return lv_Ret;
  }

} //namespace STFS
