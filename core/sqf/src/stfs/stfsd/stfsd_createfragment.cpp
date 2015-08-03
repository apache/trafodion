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
#include "stfs_fragment.h"
#include "stfs_session.h"
#include "stfsd.h"
#include "stfsd_createfragment.h"

namespace STFS {



///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_createFragment
///
/// \brief  Creates a file fragment
///         - Figures out the node of the file fragment 
///            (Currently it is the current node. TBD - check for the node)
///         - Creates a directory
///         - Creates a file in the created directory 
///         - opens the file (controlling open)
///
/// \param STFS_ExternalFileMetadata  *pp_Efm
/// \param int                         pv_OpenerNodeId,
/// \param int                         pv_OpenerPID,
/// \param long                        pv_FragmentFileOffset
/// 
///////////////////////////////////////////////////////////////////////////////
int
STFSd_createFragment(STFS_ExternalFileMetadata  *pp_Efm,
                     int                         pv_OpenerNodeId,
                     int                         pv_OpenerPID,
                     long                        pv_FragmentFileOffset)
{
  const char     *WHERE = "STFSd_createFragment";
  STFS_ScopeTrace lv_st(WHERE);

  /// housekeeping
  int lv_Ret = 0;

  if (!pp_Efm) {
    return -1;
  }

  TRACE_PRINTF5(2,
                "FileName: %s, Opener NodeId: %d, Opener SQ PID: %d, FragmentFileOffset: %ld\n",
                pp_Efm->ExternalFileNameGet(),
                pv_OpenerNodeId,
                pv_OpenerPID,
                pv_FragmentFileOffset);

  /// loop over all possible storage locations
  bool lv_moreStorageLocations = false;
  bool lv_fragmentCreated = false;

  while ((lv_moreStorageLocations != true) && (lv_fragmentCreated == false) ) {


    /// pick a storage location if one is available

    char lp_StorageLocationDir[STFS_PATH_MAX];
    bool lv_stgIsLocal = false;

    lv_Ret = STFS_fragment::GetStorageLocation (lp_StorageLocationDir,
                                                &lv_stgIsLocal);

    if (lv_Ret < 0) {
      lv_moreStorageLocations = false;         
    }

    else {

      if (lv_stgIsLocal == true){

        /// Local storage location:  Create the fragment!

        char lv_DirName[STFS_DIRNAME_MAX+1];

        bool lv_MountPointExists
                    = STFS_util::GetMountPointName(lv_DirName, 
                                                lp_StorageLocationDir); 
        ASSERT (lv_MountPointExists == true);


        /// Set up the filename
        char fragmentFilename[STFS_DIRNAME_MAX+STFS_NAME_MAX];
        STFS_fs *lp_Fs = new STFS_ext2();
        if (!lp_Fs) {
          TRACE_PRINTF1(1, "Unable to construct STFS_ext2\n");
          return -1;
        }

        size_t lv_NumFragments = pp_Efm->GetNumFragments();
        sprintf(fragmentFilename, "%s%s/f%d_", 
                lv_DirName, 
                pp_Efm->ExternalFileNameGet(),
                (short) lv_NumFragments);


        ///  Issue the mkstemp

        long lv_fsErr = lp_Fs->mkstemp(fragmentFilename);
        if (lv_fsErr < 0) {
          TRACE_PRINTF1 (1, "createFragment:FS_mkstemp is unable to create a tempfile\n");
          return lv_fsErr;
        }

        ///  Init/link the FFM  
        lv_fsErr = STFS_fragment::InitFrag (pp_Efm,
                                            lp_Fs->FilenameGet(),
                                            lp_StorageLocationDir,
                                            pv_FragmentFileOffset);
        if (lv_fsErr<0) {
          //TBD cleanup
          return -1;
        }


        /// Get the EFH for the controlling open  

        // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
        //
        // This code doesn't work for remote fragments, remote accessors of
        // local fragments, or local accessors of remote files with remote
        // fragments!  They'll need their own EFHs and this doesn't link this
        // fragment to them.  Further, the check to determin whether we're
        // creating the controlling open doesn't work if there are multiple
        // fragments but this is the first fragment on this node.
        //
        // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING



        // Get the efh for the controlling open
        STFS_ExternalFileHandle *lp_Efh = NULL;
        if ( (pp_Efm->GetNumFragments()) == 1) {
          // create the controlling open

          lp_Efh= pp_Efm->Open(true);
        }
        else {
          // get the controling open from the efm

          lp_Efh = pp_Efm->EFHGet();
        }

        if (!lp_Efh) {
          // TBD cleanup
          return -1;
        }

        /// Logically open the fragment

        lv_fsErr = STFS_fragment::Open (lp_Efh, (pp_Efm->GetNumFragments()-1), lp_Fs);

        if (lv_fsErr<0) {
          //TBD cleanup
          return -1;
        }

        lv_fragmentCreated = true;

      } // if local fragment
      else {

        /// remote fragment:  Send the request to the remote daemon
        // TBD!
        ASSERT (false);
        return -1;
      }
    }
  } // while more possible places to create frag and frag not created
  
  return lv_Ret;
}

} //namespace STFS
