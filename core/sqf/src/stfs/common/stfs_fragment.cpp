///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_fragment.cpp
///  \brief   Implementation of STFS_fragment utility
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

#include "stfs_fragment.h"
#include "stfs_util.h"
#include "stfs_sigill.h"


using namespace STFS;


///////////////////////////////////////////////////////////////////////////////
///
//          InitFrag
///
/// \brief  Does the metadata busywork for making a fragment visible
///
/// This method does the FFM busywork for making a fragment visible.  It creates
/// and allocates the FFM and links it to the current EFM.
///
/// \param pp_Efh   EFH to search for
/// 
/// \retval bool    If EFH exists in container:         True
///                 If EFH does not exist in container: False
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_fragment::InitFrag(STFS_ExternalFileMetadata *pp_Efm,
		       char *pp_FragmentFileName,
		       char *pp_StfsDirectory,
		       size_t pv_FragmentFileOffset ) {
  const char     *WHERE = "STFS_Fragment::InitFrag";
  STFS_ScopeTrace lv_st(WHERE,2);

  ///  allocate the FFM  
  STFS_FragmentFileMetadata *lp_Ffm = pp_Efm->CreateFFM(pp_FragmentFileName,
							pp_StfsDirectory,
							pv_FragmentFileOffset);
  if (!lp_Ffm) {
    // we need to report an ENOSPC error here
    ASSERT (false);
    return -1;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          OpenFrag
///
/// \brief  Does the busywork for opening a fragment
///
/// This method opens a single fragment logically, setting up the FFH and
/// linking it to the appropriate EFH.  It takes a fragment number (0 based)
/// and allocates the FFH and links it to the current EFH.
///
/// \param pp_efh      EFH for which the fragment is to be opened
/// \param pv_FragNum  The 0-based fragment which is to be opened. 
/// 
/// \retval bool    If EFH exists in container:         True
///                 If EFH does not exist in container: False
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_fragment::Open (STFS_ExternalFileHandle *pp_efh,
		     int pv_FragNum,
		     STFS_fs *pp_Fs ) {

  ///////////////////////////////
  /// Validate the parameters
  ///////////////////////////////
  if (pp_efh == NULL) {
    // Parameter error:  Don't waste my time with no efh!
    ASSERT (false);
    return -1;
  }


  /// Get the EFM

  STFS_ExternalFileMetadata *lp_efm = pp_efh->EfmGet();
  if (lp_efm == NULL) {
    // no efm for this open?  Huh?????
    ASSERT (false);
    return -1;
  }

  /// Get the FFM for the specified fragment

  STFS_FragmentFileMetadata *lp_ffm = lp_efm->GetFragment (pv_FragNum);
  if (lp_ffm == NULL) {
    // we already verified that this fragment is within range, so why isn't
    // there a fragment?
    ASSERT (false);
    return -1;
  }

  /// logically open the fragment
  STFS_FragmentFileHandle *lp_ffh = new STFS_FragmentFileHandle(lp_ffm);
  pp_efh->InsertFFH(lp_ffh);

  if (pp_Fs != NULL) {
    lp_ffh->FsSet(pp_Fs);
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetStorageLocation
///
/// \brief  Selects a storage location for a fragment
///
/// This is the method for selecting the storage location for a fragment.  This
/// is the storage location, not the name of the mountpoint at that location.
/// To get the actual mountpoint name, call STFS_Util::GetMountPointName.
///
///  
/// Currently it's a shell method that  only selects a local fragment location.
/// Eventually, the underpinnings will be used to pick an actual location on
/// different nodes if it's the controlling STFSd that's calling it.  If it's
/// not the owning STFSd, then it just picks a local storage location.
///
/// Theoretically, this is an STFSd-only method, but it's used in two very
/// different ways depending on whether the file is owned locally or not
///
/// For now, this just picks the local node.  Eventually, there should be an
/// in/out parameter that indicates the available storage locations reduced by
/// those already selected
///
/// \param pp_stgLocation    The storage location
///        pv_stgIsLocal     True is storage is local
/// 
/// \retval int     Return value
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_fragment::GetStorageLocation (char *pp_stgLocation,
				   bool *pv_stgIsLocal) {

  /// Right now, just get a local storage location.  This will be replaced by
  /// the real algorithm for picking a storage location

  strcpy (pp_stgLocation,STFS_util::GetLocalNodeStorageLocation());

  if (strlen (pp_stgLocation) == 0) {
    //  Couldn't find a storage location
    ASSERT (false);
    return -1;
  }

  // for now, storage is always local
  *pv_stgIsLocal = true;

  return 0;
}
