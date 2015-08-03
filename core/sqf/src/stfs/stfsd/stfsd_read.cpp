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
#include "stfs_message.h"
#include "stfsd.h"
#include "stfs_session.h"
#include "stfsd_read.h"

namespace STFS {


///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_read
///
/// \brief  Reads from an STFS file
///
///////////////////////////////////////////////////////////////////////////////
ssize_t
STFSd_read(STFS_OpenIdentifier *pp_OpenId,
           void                *pp_Buf,
           size_t               pv_Count)
{
  const char     *WHERE = "STFSd_read";
  STFS_ScopeTrace lv_st(WHERE);
  STFS_Session *lp_Session = STFS_Session::GetSession();

  if(!lp_Session) {
    return -1;
  }

  if(!pp_OpenId || !pp_Buf) {
    //Invalid argument errno
    return -1;
  }

  if (pv_Count > (size_t)STFS_SSIZE_MAX) {
    return -1;
  }

  //If count is > buffer size then what? Handle this at the send.cpp level

  //Lookup Open file,  Need to have added check to see if node matches
  STFS_ExternalFileOpenerContainer *lp_EfoContainer = STFS_ExternalFileOpenerContainer::GetInstance();
  if (!lp_EfoContainer) {
    TRACE_PRINTF2(1,"%s\n", "Null EFO Container");
    return -1;
  }

  STFS_ExternalFileOpenerInfo *lp_Efoi = lp_EfoContainer->Get(pp_OpenId);
  if (!lp_Efoi) {
    TRACE_PRINTF3(1,
                  "Open Id: %d,%ld not found in the EFO Container\n",
                  pp_OpenId->sqOwningDaemonNodeId,
                  pp_OpenId->openIdentifier
                  );
    return -1;
  }
  STFS_ExternalFileMetadata *lp_Efm = lp_Efoi->efm_;
  if (!lp_Efm) {
    TRACE_PRINTF1(1,"Null EFM Found in the lp_Efoi Entry\n");
    return -1;
  }

  TraceOpeners(lp_EfoContainer,
               lp_Efm);
  

  //Check to see if the file is local

  //If not local then Don't do anything. Return 0.

  //Do Read


  return 0;
}

} //namespace STFS
