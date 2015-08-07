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

  // Rev: Missing doxygen hdr
  stfs_fhndl_t
  STFSLIB_mkstemp(char *pp_Ctemplate)
  {
    const char      *WHERE = "STFSLIB_mkstemp";
    STFS_ScopeTrace lv_st(WHERE);

    // Rev: Add an ASSERT for pp_Ctemplate
    if (!pp_Ctemplate) {
      return -1;
    }

    STFS_ExternalFileMetadata *lp_Efm = 0;
    STFS_FragmentFileMetadata *lp_Ffm = 0;
    STFS_OpenIdentifier       *lp_OpenId = 0;
    int lv_ret = 0;

#ifdef SQ_PACK
    lv_ret = SendToSTFSd_mkstemp(pp_Ctemplate,
				 STFS_util::GetMyNodeId(),
				 STFS_util::GetMyPID(),
				 lp_Efm,
				 lp_OpenId);
#else
    lv_ret = STFSd_mkstemp(pp_Ctemplate,
			   STFS_util::GetMyNodeId(),
			   STFS_util::GetMyPID(),
			   lp_Efm,
			   lp_OpenId);
#endif

    // lp_OpenID was NEW'd in SendToSTFSd_mkstemp.  We need to free it
    // here after we're done.  Really, we should allocate and free at
    // the same level...

    // Rev: Check for (!= 0)
    if (lv_ret < 0) {
      // Rev: Add ASSERTs to check that lp_Efm, lp_OpenId are NULL
      delete lp_OpenId;
      return lv_ret;
    }

    // Rev: Remove this and add ASSERTs for lp_Efm/lp_OpenId being not NULL
    if (!lp_Efm) {
      delete lp_OpenId;
      return -1;
    }

    if (!lp_Efm->IsEyeCatcherValid()) {
      delete lp_OpenId;
      STFS_util::SoftwareFailureHandler(WHERE);
      return -1;
    }

    // Rev: Remove this 
    STFS_util::ValidateEFM(lp_Efm);

    STFS_ExternalFileHandle *lp_Efh = lp_Efm->Open(false);
    if (!lp_Efh) {
      //TBD cleanup
      // Rev: Send a message to the STFSd to cleanup/ delete directories
      delete lp_OpenId;
      return -1;
    }
    lp_Efh->OpenFlagsSet(O_RDWR);
    lp_Efh->OpenIdentifierSet(*lp_OpenId);

    //Rev: Valgrind shows that there is a memory leak here.  However if the following 'delete'
    //       is uncommented then our tests fail with SQ_PACK=0; and SQ_STFSD=0.  Need to figure 
    //       out what is the cause of the tests failing in order to allow this memory leak fix.
    //delete lp_OpenId;

    lp_Ffm = lp_Efm->GetFragment (0);  // This is mkstemp, so there's only one fragment!
    // Rev: Check for lp_Ffm

    STFS_FragmentFileHandle *lp_Ffh = new STFS_FragmentFileHandle(lp_Ffm);
    // Rev: Check for lp_Ffh
    lp_Efh->InsertFFH(lp_Ffh);
    //Rev: Did the Insert work?

    return (stfs_fhndl_t) lp_Efh;
  }

} // namespace STFS
