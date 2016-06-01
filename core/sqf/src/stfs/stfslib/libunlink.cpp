///////////////////////////////////////////////////////////////////////////////
// 
/// \file    libunlink.cpp
/// \brief   STFS_unlink implementation
///   
/// This file contains the implementation of the STFS_unlink() function,
/// starting with the functional STFSLIB_unlink function and drilling
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
#include "stfs_util.h"
#include "stfs_message.h"
#include "stfsd.h"

#include "send.h"

namespace STFS {


  /// \brief Delete a name and possibly the file that it refers to
  int
  STFSLIB_unlink (const char *pp_Path)
  {
    const char     *WHERE = "STFSLIB_unlink";
    STFS_ScopeTrace lv_st(WHERE);
    int             lv_Ret = 0;


    //Check that the pathname passed is not longer than our MAX
    ASSERT(strlen(pp_Path) <= STFS_PATH_MAX);

    STFS_ExternalFileMetadata *lp_Efm = STFS_ExternalFileMetadata::GetFromContainer((char *) pp_Path);

    if (lp_Efm) {
      lp_Efm->Unlink();
      if (lv_Ret < 0) {
	return -1;
      }
    }

#ifdef SQ_PACK
    lv_Ret = SendToSTFSd_unlink((char *) pp_Path,
				STFS_util::GetMyNodeId(),
				STFS_util::GetMyPID());
#else
    lv_Ret = STFSd_unlink((char *) pp_Path);
#endif

    return lv_Ret;
  }

} //namespace STFS
