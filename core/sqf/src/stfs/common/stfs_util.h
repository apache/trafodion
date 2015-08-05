#ifndef _STFS_UTIL_H
#define _STFS_UTIL_H
///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_util.h
///  \brief   Definition of STFS_Util Class
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

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

// For EV Log
#include "sqevlog/evl_sqlog_writer.h"
#include "common/evl_sqlog_eventnum.h"

#include "stfs_defs.h"
#include "stfs_metadata.h"

namespace STFS {
class STFS_ExternalFileHandle;
class STFS_ExternalFileMetadata;
class STFS_FragmentFileMetadata;
struct STFS_OpenersSet;
}

using namespace STFS;


namespace STFS {

void
STFSd_exit();

///////////////////////////////////////////////////////////////////////////////
///
/// \class  STFS_Util
///
/// \brief  STFS Utility class
///
/// This class contains utility methods to help with the STFS implementation.
///  All the methods/data members in this class are static.
///
///////////////////////////////////////////////////////////////////////////////
class STFS_util {
 public:

  // Size of EV Log buffer
  enum { STFS_EVLOG_BUF_SIZE = 4096 };

  // Rev: Move to a static in the EFH class
 // Check whether the EFH exists
 static bool EFHExists(STFS_ExternalFileHandle *pp_Efh);

 // To Generate the STFS External File Name
 static int GenerateExternalFilename(char *pp_FileName);

 // Returns Node Id of the SQ Node 
 static int GetMyNodeId();

 // Returns my SQ PID
 static int GetMyPID();

 // Searches for a storage location on the current node
 static char *GetLocalNodeStorageLocation();

 //Populates the OpenId for an open/mkstemp
 static int GetOpenId(STFS_OpenIdentifier& pv_OpenId);

 // Initialize STFS environment
 static void  Init(bool pv_ExecutingInDaemon=true);

 // Rev: Move to a static in the EFM class
 // Validate the EFM
 static bool ValidateEFM(STFS_ExternalFileMetadata *pp_Efm);

 //  Compress/uncompress file names for sending messages and other uses
 static size_t CompressString (char     *pp_ExpandedString,
                               char     *pp_tgtLocation,
                               size_t    pv_tgtMaxSize);

 static size_t UncompressString (char     *pp_compString,
                                 char     *pp_tgtString,
                                 size_t    pv_tgtMaxSize);


 // Handle Software Failure
 static void SoftwareFailureHandler(const char * pp_Where);

 // Create stfs directory for temp files
 static int CreateDir();

 static int RemoveDir(char* pp_DirName);

 static bool GetMountPointName (char *pp_MountpointName,
				char * pp_StorageLocation);
 
 // Rev: Change the names of the next 4 methods so that it's diferent from the user-exposed ones
 static int STFS_openers(stfs_nodeid_t     pv_nid,      
                         char             *pp_path,    
                         long             *pp_previndex,   
                         stfs_nodeid_t    &pv_openernid,  
                         pid_t            &pv_openerpid, 
                         char             *pp_openername);

 static int STFS_fopeners(stfs_fhndl_t     pv_Fhandle,      
                          long            *pp_PrevIndex,    
                          stfs_nodeid_t   &pv_OpenerNid,  
                          pid_t           &pv_OpenerPid,  
                          char            *pp_OpenerName, 
                          char            *pp_OpenPath);

 static int STFS_stat(stfs_nodeid_t     pv_Nid,
                      char             *pp_Path,
                      long             *pp_PrevIndex,
                      stfs_statmask_t   pv_Mask,
                      struct stfs_stat *pp_Buf); 

 static int STFS_fstat(stfs_fhndl_t      pv_Fhandle,
                       stfs_statmask_t   pv_Mask,
                       struct stfs_stat *pp_Buf); 

 static int STFS_ping();

#ifdef SQ_STFSD
 static int SendToSTFSd(void *pp_ReqBuffer,
                        int   pv_ReqLen,
                        void *pp_RspBuffer,
                        int  *pp_RspLen);
#endif

 static int STFS_evlog(int                     pv_EventType,
		       posix_sqlog_severity_t  pv_Severity,
		       char                   *pp_EvlBuf);

 static char  currentDir_[512];
 static int   currentNodeId_;
 static long  currentOpenId_;
 static short initDone_;
 static int   myPID_;
 static short traceLevel_;
 static bool  executingInDaemon_;
 static bool  seaquestFilesystemInitialized_;
};

///////////////////////////////////////////////////////////////////////////////
///
/// \class  STFS_ScopeTrace
///
/// \brief  Provides a simple way to trace scope entry/exit
///
/// Usage: STFS_ScopeTrace lv_st("<scope name>", <trace level>);
///
///////////////////////////////////////////////////////////////////////////////
class STFS_ScopeTrace {

public:
  STFS_ScopeTrace(const char *pp_ScopeName, short pv_TraceLevel = 1);

  ~STFS_ScopeTrace();

  const char *ScopeNameGet() const {return scopeName_; }
  
private:
  const char *scopeName_;
  short       scopeTraceLevel_;

};

}

#endif
