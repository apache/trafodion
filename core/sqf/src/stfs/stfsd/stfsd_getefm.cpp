///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfsd_getefm.cpp
/// \brief  
///   
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
#include "stfsd_getefm.h"

namespace STFS {



///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_getEFM()
///
/// \brief  Gets EFM from daemon
///
/// \param STFS_OpenIdentifier        *pp_OpenId
/// \param STFS_ExternalFileMetadata  *pp_Efm
/// \param int                         pv_OpenerNodeId
/// \param int                         pv_OpenerPID
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_getEfm ( STFS_OpenIdentifier        *pp_OpenId,
               STFS_ExternalFileMetadata  *pp_Efm,
               int                         pv_OpenerNodeId,
               int                         pv_OpenerPID)
{
  const char     *WHERE = "STFSd_getEfm";
  STFS_ScopeTrace lv_st(WHERE);

  /// housekeeping
  int lv_Ret = 0;

  if (!pp_Efm || !pp_OpenId) {
    return -1;
  }

  TRACE_PRINTF4(2,
                "OpenID: %ld, Opener NodeId: %d, Opener SQ PID: %d\n",
                pp_OpenId->openIdentifier,
                pv_OpenerNodeId,
                pv_OpenerPID);

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
  pp_Efm = lp_Efoi->efm_;
  if (!pp_Efm) {
    TRACE_PRINTF1(1,"Null EFM Found in the Efoi Entry\n");
    return -1;
  }

  
  return lv_Ret;
}
 
} //namespace STFS
