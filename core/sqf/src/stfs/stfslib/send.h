#ifndef STFSLIB_SEND_H
#define STFSLIB_SEND_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    send.h
///  \brief   Header file for STFLIB send-associated functionality
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

  int SendToSTFSd_close (STFS_OpenIdentifier     *pp_OpenId,
			 int                      pv_OpenerNodeId,
			 int                      pv_openerPID);

  int  SendToSTFSd_unlink( const char            *pp_Path,
			   int                    pv_OpenerNodeId,
			   int                    pv_OpenerPID);

  int SendToSTFSd_mkstemp(char                       *pp_Ctemplate,
			  int                         pv_OpenerNodeId,
			  int                         pv_OpenerPID,
			  STFS_ExternalFileMetadata *&pp_Efm,
			  STFS_OpenIdentifier       *&pp_OpenId);

  int SendToSTFSd_open(char                       *pp_FileName,
		       int                         pv_OpenFlag,
		       int                         pv_OpenerNodeId,
		       int                         pv_OpenerPID,
		       STFS_ExternalFileMetadata *&pp_Efm,
		       STFS_FragmentFileMetadata *&pp_Ffm,
		       STFS_OpenIdentifier       *&pp_OpenId);

  int
  SendToSTFSd_read();
  
  int SendToSTFSd_createFrag (STFS_ExternalFileMetadata *&pp_Efm,
			      int                         pv_OpenerNodeId,
			      int                         pv_OpenerPID,
			      STFS_OpenIdentifier        *pp_OpenId);


  int SendToSTFSd_fopeners(stfs_fhndl_t                pv_Fhandle,      
                           struct STFS_OpenersSet     *pp_OpenersSet,
                           int                         pv_OpenerNodeId,
                           int                         pv_OpenerPID);

  int SendToSTFSd_openers(stfs_nodeid_t               pv_Nid,
			  char                       *pp_Path,
			  struct STFS_OpenersSet      pp_OpenersSet[],
			  int                         pv_OpenerNodeId,
			  int                         pv_OpenerPID);
  int
  SendToSTFSd_stat ( stfs_nodeid_t               pv_Nid,
                     char                       *pp_Path,
                     struct STFS_StatSet        *pp_StatSet,
                     stfs_statmask_t             pv_Mask, 
                     int                         pv_OpenerNodeId,
                     int                         pv_OpenerPID);

} //namespace STFS;

#endif // STFSLIB_SEND_H
