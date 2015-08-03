///////////////////////////////////////////////////////////////////////////////
// 
/// \file    libfcntl.cpp
/// \brief   STFS_fcntl implementation
///   
/// This file contains the implementation of the STFS_fcntl() function,
/// starting with the functional STFSLIB_fcntl function and drilling
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

  int 
  STFSLIB_fcntl( stfs_fhndl_t pv_Fhandle, 
		 int          pv_Cmd)
  {
    //Get Flags
    if((pv_Cmd & F_GETFL) != F_GETFL) {
      return -1;
    }

    STFS_ExternalFileHandle *lp_Efh = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
    if (!lp_Efh) {
      //TBD cleanup
      return -1;
    }

    lp_Efh->ResetErrors();

  
    return lp_Efh->OpenFlagsGet();
  }

  int 
  STFSLIB_fcntl( stfs_fhndl_t pv_Fhandle, 
		 int          pv_Cmd,
		 long         pv_Flag)
  {
    //Set Flag
    if((pv_Cmd & F_SETFL) != F_SETFL) {
      return -1;
    }

    STFS_ExternalFileHandle *lp_Efh = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
    if (!lp_Efh) {
      //TBD cleanup
      return -1;
    }

    lp_Efh->ResetErrors();

    lp_Efh->OpenFlagsSet(pv_Flag);
    return 0;
  }

} //namespace STFS
