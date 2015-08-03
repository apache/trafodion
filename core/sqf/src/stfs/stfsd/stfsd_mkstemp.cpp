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
#include "stfs_fragment.h"
#include "stfs_file.h"
#include "stfs_message.h"
#include "stfs_session.h"
#include "stfsd.h"
#include "stfsd_mkstemp.h"

namespace STFS {
///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_mkstemp
///
/// \brief  Processes an mkstemp request
///         - Generates an external file name
///         
/// \param int                        *pp_Ctemplate,
/// \param int                         pv_OpenerNodeId,
/// \param int                         pv_OpenerPID,
/// \param STFS_ExternalFileMetadata *&pp_Efm,
/// \param STFS_OpenIdentifier       *&pp_OpenId)
///
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_mkstemp(char                       *pp_Ctemplate,
              int                         pv_OpenerNodeId,
              int                         pv_OpenerPID,
              STFS_ExternalFileMetadata *&pp_Efm,
              STFS_OpenIdentifier       *&pp_OpenId)
{

  /////////////////////////////////
  /// parameter checking and housekeeping
  /////////////////////////////////

  const char     *WHERE = "STFSd_mkstemp";
  STFS_ScopeTrace lv_st(WHERE);
  STFS_Session *lp_Session = STFS_Session::GetSession();

  int  lv_Ret = 0;

  if (!pp_Ctemplate) {
    errno = EFAULT;
    lp_Session->SetError(true, errno, 0, NULL, 0);
    return -1;
  }

  /// Generate the file name
  lv_Ret = STFS_util::GenerateExternalFilename(pp_Ctemplate);
  if (lv_Ret < 0) {
    errno = EINVAL;
    char lp_ErrMess[] = "Error generating external file name";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -2;
  }

  /////////////////////////////////
  /// Reserve the name
  /////////////////////////////////

  // We're the owning daemon process, so we need to reserve the name
  lv_Ret =  STFS_File::ReserveName (pp_Ctemplate);

  if (lv_Ret<0) {
    errno = EIO;

    char lp_ErrMess[] = "Error while reserving name";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    
    //name exists?  Couldn't talk to the disk?  WhatEVer...  Hope that
    //ReserveName packed the error correctly...
    return -6;

  }

  /////////////////////////////////
  /// Create the EFM for the file
  /////////////////////////////////

  STFS_ExternalFileMetadata *lp_Efm = new STFS_ExternalFileMetadata(pp_Ctemplate,
                                                                    STFS_util::GetMyNodeId());

  if (lp_Efm == NULL) {

    //Could not find/allocate EFM, probably doesn't exist
    errno = ENOENT;
    char lp_ErrMess[] = "Error allocating EFM";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));

    //UhOh, couldn't allocate an EFM!  Gotta free the directory we created
    STFS_File::ReleaseName (pp_Ctemplate); // 
    return -7;
  }

  TRACE_PRINTF4(2,
                "FileName: %s, Opener Node Id: %d, Opener SQ PID: %d\n",
                lp_Efm->ExternalFileNameGet(),
                pv_OpenerNodeId,
                pv_OpenerPID);

  /////////////////////////////////
  ///insert EFM in a global map
  /////////////////////////////////

  lv_Ret = STFS_ExternalFileMetadata::InsertIntoContainer(lp_Efm);
  if (lv_Ret < 0) {
    //TBD cleanup
    //Could not insert into EFM Container... 
    errno = ENOMEM;
    char lp_ErrMess[] = "Error Insertnig EFM into EFM Container";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -4;
  }

  /////////////////////////////////
  /// Store the opener info in the EFM
  /////////////////////////////////

  lv_Ret = StoreOpenerInfo(lp_Efm,
                           pv_OpenerNodeId,
                           pv_OpenerPID,
                           pp_OpenId);
  if (lv_Ret < 0) {
    //TBD cleanup
    //Could not store opener information
    errno = ENOMEM;
    char lp_ErrMess[] = "Error storing opener information";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -5;
  }

  /////////////////////////////////
  /// Create the fragment
  /////////////////////////////////

  if (STFSd_createFragment(lp_Efm,
                           pv_OpenerNodeId,
                           pv_OpenerPID,
                           0) < 0) {
    //Could not create fragment
    errno = EIO;
    char lp_ErrMess[] = "Error creating fragment";
    lp_Session->SetError(true, errno, 0, lp_ErrMess, strlen(lp_ErrMess));
    return -3;
  }


  STFS_util::ValidateEFM(lp_Efm);


  pp_Efm = lp_Efm;
  return 0;
}

} //namespace STFS
