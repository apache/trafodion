///////////////////////////////////////////////////////////////////////////////
// 
/// \file    libmkstemp.cpp
/// \brief   STFS_mkstemp implementation
///   
/// This file contains the implementation of the STFS_mkstemp() function,
/// starting with the functional STFSLIB_mkstemp function and drilling
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


  stfs_fhndl_t 
  STFSLIB_open( const char *pp_Path,    
		int         pv_OFlag)
  {
    const char     *WHERE = "STFSLIB_open";
    STFS_ScopeTrace lv_st(WHERE);

    if(!pp_Path) {
      TRACE_PRINTF1(1,"Error Path parameter is NULL\n");
      return -1;
    }

    ASSERT(strlen(pp_Path) <= STFS_PATH_MAX);

    STFS_ExternalFileMetadata *lp_Efm = 0;
    STFS_FragmentFileMetadata *lp_Ffm = 0;
    STFS_OpenIdentifier       *lp_OpenId = 0;
    int                        lv_Ret = 0;
  
#ifdef SQ_PACK
    lv_Ret = SendToSTFSd_open((char *)pp_Path,
			      pv_OFlag, 
			      STFS_util::GetMyNodeId(),
			      STFS_util::GetMyPID(),
			      lp_Efm,
			      lp_Ffm, 
			      lp_OpenId);
#else
    lv_Ret = STFSd_open(pp_Path,
			pv_OFlag, 
			STFS_util::GetMyNodeId(),
			STFS_util::GetMyPID(),
			lp_Efm,
			lp_Ffm, 
			lp_OpenId);
#endif
    if (lv_Ret < 0) {
      TRACE_PRINTF1(1,"Error STFSd_open returned -1\n");
      return lv_Ret;
    }

    if (!lp_Efm) {
      TRACE_PRINTF1(1,"Error EFM is NULL\n");
      return -1;
    }

    if (!lp_Efm->IsEyeCatcherValid()) {
      STFS_util::SoftwareFailureHandler(WHERE);
      return -1;
    }

    STFS_util::ValidateEFM(lp_Efm);

    STFS_ExternalFileHandle *lp_Efh = lp_Efm->Open(false);
    if (!lp_Efh) {
      TRACE_PRINTF1(1,"Error obtaining EFH\n");
      //TBD cleanup
      return -1;
    }
    lp_Efh->OpenFlagsSet(pv_OFlag);
    lp_Efh->OpenIdentifierSet(*lp_OpenId);

    STFS_FragmentFileHandle *lp_Ffh = new STFS_FragmentFileHandle(lp_Ffm);
    lp_Efh->InsertFFH(lp_Ffh);

    return (stfs_fhndl_t) lp_Efh;
  }

} // namespace STFS
