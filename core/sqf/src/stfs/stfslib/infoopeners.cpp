///////////////////////////////////////////////////////////////////////////////
// 
/// \file    infoopeners.cpp
/// \brief   STFS_openers and STFS_fopeners implementation
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
  int STFSINFO_fopeners(stfs_fhndl_t     pv_Fhandle,      
                        long            *pp_PrevIndex,    
                        stfs_nodeid_t   &pv_OpenerNid,  
                        pid_t           &pv_OpenerPid,  
                        char            *pp_OpenerName, 
                        char            *pp_OpenPath)
  {
    const char     *WHERE = "STFSLIB_fopeners";
    STFS_ScopeTrace lv_st(WHERE);
    int lv_Ret = 0;
    static STFS_OpenersSet lv_OpenersSet;

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
      
    
    if(*pp_PrevIndex == 0)
      {
        //If first call to this function then call the Daemon function
        //to get the information If there is at least one element in
        //the OpenersSet then we set the values and return 0
        //Otherwise, find the appropriate value to return to caller
        //using pp_previndex

#ifdef SQ_PACK
        lv_Ret = SendToSTFSd_openers(-1,
                                     lv_FileName,
                                    &lv_OpenersSet,
                                     STFS_util::GetMyNodeId(),
                                     STFS_util::GetMyPID());
#else
        lv_Ret = STFSd_openers(-1, 
                               lv_FileName,
                              &lv_OpenersSet);
#endif
      }

    if(lv_Ret < 0) {
    // TODO: Error Checking
      return -1;
    }
    if((*pp_PrevIndex) == lv_OpenersSet.Count) {
      // We returned the last element already. Return 1 to signify no more openers.
      return 1;
    }

    //Setting the output parameters
    strcpy(pp_OpenPath, lv_OpenersSet.Path[*pp_PrevIndex]);
    pv_OpenerNid  = lv_OpenersSet.Nid[*pp_PrevIndex];
    pv_OpenerPid  = lv_OpenersSet.Pid[*pp_PrevIndex];
    strcpy(pp_OpenerName, lv_OpenersSet.OpenerName[*pp_PrevIndex]);

    *pp_PrevIndex += 1;
    return lv_Ret;
  }

  int STFSINFO_openers(stfs_nodeid_t     pv_nid,      
                       char             *pp_path,    
                       long             *pp_previndex,   
                       stfs_nodeid_t    &pv_openernid,  
                       pid_t            &pv_openerpid, 
                       char             *pp_openername)
  {
    const char     *WHERE = "STFSLIB_openers";
    STFS_ScopeTrace lv_st(WHERE);
    int lv_Ret = 0;
    //TODO:Clear out the struct
    static STFS_OpenersSet lv_OpenersSet;

    //static variable of a structure that holds the openers
    //static struct OpenersStruct pv_OpenersStruct;
    if(*pp_previndex == 0)
      {
        //If first call to this function then call the Daemon function
        //to get the information If there is at least one element in
        //the OpenersSet then we set the values and return 0
        //Otherwise, find the appropriate value to return to caller
        //using pp_previndex

#ifdef SQ_PACK
        lv_Ret = SendToSTFSd_openers(pv_nid,
                                     pp_path,
                                    &lv_OpenersSet,
                                     STFS_util::GetMyNodeId(),
                                     STFS_util::GetMyPID());
#else
        lv_Ret = STFSd_openers(pv_nid, 
                               pp_path,
                              &lv_OpenersSet);
#endif
      }

    if(lv_Ret < 0) {
    // TODO: Error Checking
      return -1;
    }
    if((*pp_previndex) == lv_OpenersSet.Count) {
      // We returned the last element already. Return 1 to signify no more openers.
      return 1;
    }
    //Setting the output parameters
    strcpy(pp_path, lv_OpenersSet.Path[*pp_previndex]);
    pv_openernid  = lv_OpenersSet.Nid[*pp_previndex];
    pv_openerpid  = lv_OpenersSet.Pid[*pp_previndex];
    strcpy(pp_openername, lv_OpenersSet.OpenerName[*pp_previndex]);

    *pp_previndex += 1;
    return lv_Ret;
  }

} //namespace STFS
