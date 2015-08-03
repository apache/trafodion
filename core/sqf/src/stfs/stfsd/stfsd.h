#ifndef STFSD_H
#define STFSD_H
///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfsd.h
///  \brief   STFS Daemon header
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

#include "stfsdcalls.h"

namespace STFS {

void
TraceOpeners(STFS_ExternalFileOpenerContainer *pp_EfoContainer,
             STFS_ExternalFileMetadata        *pp_Efm);

int 
StoreOpenerInfo(STFS_ExternalFileMetadata *pp_Efm,
                int                        pv_OpenerNodeId,
                int                        pv_OpenerPID,
                STFS_OpenIdentifier      *&pp_OpenId);

void
STFSd_exit();

int
STFSd_Startup();

} // namespace STFS

#endif //STFSD_H
