#ifndef STFS_INFOOPENERS_H
#define STFS_INFOOPENERS_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    infoopeners.h
///  \brief   Header file for STFSinfo-openers-associated functionality
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

namespace STFS {

  /////////////////////////////////////
  /// function protototypes
  /////////////////////////////////////

  int STFSINFO_fopeners(stfs_fhndl_t     pv_Fhandle,      
                        long            *pp_PrevIndex,    
                        stfs_nodeid_t   &pv_OpenerNid,  
                        pid_t           &pv_OpenerPid,  
                        char            *pp_OpenerName, 
                        char            *pp_OpenPath);

  int STFSINFO_openers (stfs_nodeid_t    pv_nid,
			char            *pp_path,
			long            *pp_previndex,
			stfs_nodeid_t   &pv_openerid,
			pid_t           &pv_openerpid,
			char            *pp_openername);

} //namespace STFS;

#endif // STFS_INFOOPENERS_H
