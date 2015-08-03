///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfsd_open.cpp
/// \brief  
///   
/// This file contains the implementation of the STFSd_open() 
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
#include "stfsd.h"
#include "stfs_session.h"
#include "stfsd_open.h"

namespace STFS {

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_open
///
/// \brief  Opens an existing STFS file
///
/// \param const char                *pp_Path, 
/// \param int                        pv_OFlag, 
/// \param                            pv_OpenerNodeId,
/// \param                            pv_OpenerPID,
/// \param STFS_ExternalFileMetadata *&pp_Efm,
/// \param STFS_FragmentFileMetadata *&pp_Ffm,
/// \param STFS_OpenIdentifier       *&pp_OpenId)
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_open(const char                 *pp_Path, 
           int                         pv_OFlag, 
           int                         pv_OpenerNodeId,
           int                         pv_OpenerPID,
           STFS_ExternalFileMetadata *&pp_Efm,
           STFS_FragmentFileMetadata *&pp_Ffm,
           STFS_OpenIdentifier       *&pp_OpenId)
{
  const char     *WHERE = "STFSd_open";
  STFS_ScopeTrace lv_st(WHERE);
  STFS_Session *lp_Session = STFS_Session::GetSession();

  int  lv_Ret = 0;

  if (!pp_Path) {
    errno = EINVAL;
    char lp_ErrMess[] = "Error Path is NULL";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -1;
  }

  char lp_Path[STFS_PATH_MAX];
  strcpy(lp_Path, pp_Path);
  STFS_ExternalFileMetadata *lp_Efm = STFS_ExternalFileMetadata::GetFromContainer(lp_Path);
  if(!lp_Efm) {
    TRACE_PRINTF2(1,"%s\n", "Error getting lp_Efm");

    errno = EINVAL;
    char lp_ErrMess[] = "Error getting lp_Efm";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -2;
  }
  
  TRACE_PRINTF4(2,
                "FileName: %s, Opener Node Id: %d, Opener SQ PID: %d\n",
                lp_Efm->ExternalFileNameGet(),
                pv_OpenerNodeId,
                pv_OpenerPID);

  STFS_util::ValidateEFM(lp_Efm);

  if(!lp_Efm->FileAvailableGet()) {
    TRACE_PRINTF2(1, "File: %s is no longer available\n", pp_Path);
    errno = ENOENT;
    char lp_ErrMess[] = "Error file is no longer available";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -1;
  }

  int lv_FragmentOffset = 0;
  //If O_APPEND, last fragment will be set open,
  if((pv_OFlag & O_APPEND) == O_APPEND) {
    lv_FragmentOffset = lp_Efm->GetNumFragments();
    if(lv_FragmentOffset == 0) {
      TRACE_PRINTF2(1,"%s\n", "Error Number of Fragments returned is 0");
      errno = ENOENT;
      char lp_ErrMess[] = "Error number of fragments returned is 0";
      lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
      return -3;
    }
    lv_FragmentOffset -= 1;
  }

  STFS_FragmentFileMetadata *lp_Ffm = lp_Efm->GetFragment(lv_FragmentOffset);
  if(!lp_Ffm) {
    errno = ENOENT;
    char lp_ErrMess[] = "Error obtaining fragment";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -4;
  }

  STFS_ExternalFileHandle *lp_Efh = lp_Efm->Open(true);
  if (!lp_Efh) {
    // TBD cleanup
    errno = ENOENT;
    char lp_ErrMess[] = "Error allocating EFH";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -5;
  }

  lv_Ret = StoreOpenerInfo(lp_Efm,
                           pv_OpenerNodeId,
                           pv_OpenerPID,
                           pp_OpenId);
  if (lv_Ret < 0) {
    //TBD cleanup
    errno = ENOENT;
    char lp_ErrMess[] = "Error storing opener information";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -6;
  }

  pp_Efm = lp_Efm;
  pp_Ffm = lp_Ffm;
  
  //Build Success Reply
  return 0;
}

} //namespace STFS
