///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfsd_unlink.cpp
/// \brief  
///   
/// This file contains the implementation of the STFSd_unlink() 
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
#include "stfsd_createfragment.h"

namespace STFS {


///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_unlink
///
/// \brief  Unlinks an STFS file given its name
///
/// \param  char    *pp_Path(STFS File Name)
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_unlink(char    *pp_Path)
{
  const char     *WHERE = "STFSd_unlink";
  STFS_ScopeTrace lv_st(WHERE);

  //Check that the pathname passed is not longer than our MAX
  ASSERT(strlen(pp_Path) <= STFS_PATH_MAX);

  STFS_ExternalFileMetadata *lp_Efm = STFS_ExternalFileMetadata::GetFromContainer(pp_Path);
  if (!lp_Efm) {
    TRACE_PRINTF2(1, "EFM: %s not found in the EFM Container\n", pp_Path);
    errno = ENOENT;
    return -1;
  }

  int lv_Ret = lp_Efm->Unlink();

  return lv_Ret;
}

} // namespace STFS
