///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfsd.cpp
///  \brief   STFS Daemon Code
/// 
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
#include "seabed/trace.h"

#include "stfs_metadata.h"
#include "stfs_defs.h"
#include "stfs_util.h"
#include "stfs_fragment.h"
#include "stfs_file.h"
#include "stfs_message.h"
#include "stfs_session.h"
#include "stfsd.h"

using namespace STFS;
namespace STFS {
///////////////////////////////////////////////////////////////////////////////
///
//          TraceOpeners
///
/// \brief  A helper method to trace the openers for a particular file 
///         (referred in pp_Efm)
///
/// \param[in]   STFS_ExternalFileOpenerContainer *pp_EfoContainer
/// \param[in]   STFS_ExternalFileMetadata        *pp_Efm
/// 
///////////////////////////////////////////////////////////////////////////////
void
TraceOpeners(STFS_ExternalFileOpenerContainer *pp_EfoContainer,
             STFS_ExternalFileMetadata        *pp_Efm)
{
  char *lp_ExternalFileName = pp_Efm->ExternalFileNameGet();
  long lv_NumOpeners = pp_EfoContainer->Size(lp_ExternalFileName);
  TRACE_PRINTF3(4, "Num openers of %s: %ld\n", lp_ExternalFileName, lv_NumOpeners);
  STFS_ExternalFileOpener* lp_Efo = pp_EfoContainer->Get(lp_ExternalFileName, 0);
  if (lp_Efo) {
    TRACE_PRINTF4(4, "Opener of %s: node: %d, pid: %d\n",
                  lp_ExternalFileName,
                  lp_Efo->sqOpenerNodeId_,
                  lp_Efo->sqOpenerPID_);

  }
}

///////////////////////////////////////////////////////////////////////////////
///
//          StoreOpenerInfo
///
/// \brief  Generates an open id and stores the information about a file 
///         opener in the STFS_ExternalFileOpenerContainer
///
/// \param STFS_ExternalFileMetadata *pp_Efm
/// \param int                        pv_OpenerNodeId
/// \param int                        pv_OpenerPID,
/// \param STFS_OpenIdentifier      *&pp_OpenId  (the generated open id)
/// 
///////////////////////////////////////////////////////////////////////////////
int 
StoreOpenerInfo(STFS_ExternalFileMetadata *pp_Efm,
                int                        pv_OpenerNodeId,
                int                        pv_OpenerPID,
                STFS_OpenIdentifier      *&pp_OpenId)
{
  const char       *WHERE = "StoreOpenerInfo";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Ret = 0;

  STFS_OpenIdentifier *lp_OpenId = new STFS_OpenIdentifier();
  if (!lp_OpenId) {
    TRACE_PRINTF1(1,"Error while allocating STFS_OpenIdentifier\n");
    // TBD Cleanup
    return -1;
  }

  STFS_util::GetOpenId(*lp_OpenId);

  STFS_ExternalFileOpenerInfo *lp_FileOpenerInfo = new STFS_ExternalFileOpenerInfo();
  if (!lp_FileOpenerInfo) {
    TRACE_PRINTF1(1,"Error while allocating STFS_ExternalFileOpenerInfo\n");
    // TBD Cleanup
    return -1;
  }

  lp_FileOpenerInfo->efo_.sqOpenerNodeId_ = pv_OpenerNodeId;
  lp_FileOpenerInfo->efo_.sqOpenerPID_ = pv_OpenerPID;
  lp_FileOpenerInfo->efm_ = pp_Efm;

  STFS_ExternalFileOpenerContainer *lp_EfoContainer = STFS_ExternalFileOpenerContainer::GetInstance();
  if (!lp_EfoContainer) {
    TRACE_PRINTF1(1,"Error while allocating STFS_OpenIdentifier\n");
    // TBD Cleanup
    return -1;
  }

  lv_Ret = lp_EfoContainer->Insert(lp_OpenId,
                                   lp_FileOpenerInfo);
  if (lv_Ret < 0) {
    //TBD cleanup
    return -1;
  }
  
  pp_OpenId = lp_OpenId;
  return lv_Ret;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_exit
///
/// \brief  Is invoked by the OS when the STFSd process is about to exit.
///         It cycles through the existing EFMs and closes them.
///
/// 
///////////////////////////////////////////////////////////////////////////////
void
STFSd_exit()
{
  const char       *WHERE = "STFSd_exit";
  STFS_ScopeTrace   lv_st(WHERE,2);

  STFS_ExternalFileMetadataContainer *lp_EfmContainer = STFS_ExternalFileMetadata::GetContainer();
  if (!lp_EfmContainer) {
    TRACE_PRINTF2(1, "%s\n", "Null EFM Container");
  }
  
  //Loop through all the EFMs and close them
  lp_EfmContainer->Cleanup();

#ifdef SQ_STFSD
  if (STFS_util::executingInDaemon_) {
#endif
  char lv_buf[256];
  sprintf(lv_buf, "STFS Shutdown on Node: %d\n", STFS_util::GetMyNodeId());
  TRACE_PRINTF1(1, lv_buf);
  STFS_util::STFS_evlog(STFS_EVENT_SHUTDOWN,
                        SQ_LOG_INFO,
                        (char *) &lv_buf);
#ifdef SQ_STFSD
  }
#endif

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_Startup
///
/// \brief  Gets called on startup by the SQ monitor
///
/// \retval int (o on success)
///
///////////////////////////////////////////////////////////////////////////////
int
STFSd_Startup()
{
  const char       *WHERE = "STFSd_Startup";
  STFS_ScopeTrace   lv_st(WHERE,2);

  STFS_util::Init(true);

  return 0;
}
}
