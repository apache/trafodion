///////////////////////////////////////////////////////////////////////////////
// 
/// \file    liblseek.cpp
/// \brief   STFS_lseek implementation
///   
/// This file contains the implementation of the STFS_lseek() function,
/// starting with the functional STFSLIB_lseek function and drilling
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

namespace STFS {


  ssize_t
  STFSLIB_lseek( stfs_fhndl_t  pv_Fhandle,
		 off_t         pv_Offset,
		 int           pv_Whence)
  {
    const char     *WHERE = "STFSLIB_lseek";
    STFS_ScopeTrace lv_st(WHERE);

    STFS_ExternalFileHandle *lp_Efh = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
    if (! lp_Efh) {
      TRACE_PRINTF2(1, "Error in obtaining External File Handle for %ld\n", pv_Fhandle);
      return -1;
    }

    lp_Efh->ResetErrors();

    ssize_t lv_Stat = lp_Efh->Lseek(pv_Offset,
				    pv_Whence);
    if (lv_Stat < 0) {
      return -1;
    }

    return lv_Stat;
  }

} //namespace STFS
