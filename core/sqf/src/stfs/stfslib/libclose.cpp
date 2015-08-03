///////////////////////////////////////////////////////////////////////////////
// 
/// \file    libclose.cpp
/// \brief   STFS_close implementation
///   
/// This file contains the implementation of the STFS_close() function,
/// starting with the functional STFSLIB_close function and drilling
/// down to supporting functions.
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
#include "stfs_message.h"
#include "stfs_util.h"
#include "stfsd.h"

#include "send.h"

namespace STFS {

  int
  STFSLIB_close(stfs_fhndl_t pv_Fhandle)
  {
    const char     *WHERE = "STFSLIB_close";
    STFS_ScopeTrace lv_st(WHERE);

    STFS_ExternalFileHandle *lp_Efh = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
    if (! lp_Efh) {
      errno = EBADF;
      TRACE_PRINTF2(1, "Error in obtaining External File Handle for %ld\n", pv_Fhandle);
      return -1;
    }
    
    lp_Efh->ResetErrors();

    /// Rev: Do we close . Set the state. Continue on errors. 

    // Closes the fragments
    int lv_Ret = lp_Efh->Close();
    if (lv_Ret < 0) {
      TRACE_PRINTF3(1, "Error %d while closing handle: %ld\n",
		   lv_Ret,
		   pv_Fhandle);
      // continue on error when closing
    }

    STFS_ExternalFileMetadata *lp_Efm = lp_Efh->EfmGet();
    if (!lp_Efm) {
      // corruption in some STFS maintained data structures. 
      STFS_util::SoftwareFailureHandler(WHERE);
      return -1;
    }

    lv_Ret = lp_Efm->Close(false);
    if (lv_Ret < 0) {
      TRACE_PRINTF3(1,"Error %d while closing handle: %ld\n",
		   lv_Ret,
		   pv_Fhandle);
      return -1;
    }

    lv_Ret = STFS_ExternalFileMetadata::DeleteFromContainer(lp_Efm);
    if (lv_Ret < 0) {
      STFS_util::SoftwareFailureHandler(WHERE);
      return lv_Ret;
    }

    // send message to the STFS daemon
#ifdef SQ_PACK
    lv_Ret = SendToSTFSd_close(&(lp_Efh->OpenIdentifierGet()),
                               STFS_util::GetMyNodeId(),
                               STFS_util::GetMyPID());
    if (lv_Ret < 0) {
      // 'errno' should be set here
      return lv_Ret;
    }
#else
    lv_Ret = STFSd_close(&(lp_Efh->OpenIdentifierGet()));
    if (lv_Ret < 0) {
      return lv_Ret;
    }
#endif

    // remove Efh from the Container
    lv_Ret = STFS_ExternalFileHandle::DeleteFromContainer(lp_Efh);
    if (lv_Ret < 0) {
      STFS_util::SoftwareFailureHandler(WHERE);
      return lv_Ret;
    }

    return 0;
  }

}//namespace STFS
