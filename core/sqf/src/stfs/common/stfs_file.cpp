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

#include "stfs_file.h"
#include "stfs_util.h"
#include "stfs_sigill.h"


using namespace STFS;

///////////////////////////////////////////////////////////////////////////////
///
//          OpenFile
///
/// \brief  Does a logical open of a file
///
/// This method does the processing related to the logical opening of an STFS
/// file.  "Logical" opening means setting up the EFH appropriately and bumping
/// the EFM's usage count.   This method does not explicitly open any fragment.
///
/// At the end of an OpenFile call, the EFH is allocated and ready to be used
/// for I/O operations for this open.
///
/// This method presumes that the file is already created and the EFM is
/// accessible. It takes the EFM as an argument to save repeated EFM lookups.
///  
/// \param pp_eFM        The EFM for the file to be opened
/// \param pp_openFlags  The open flags for the file.
/// 
/// \retval RetVal < 0   File is not open and EFH cannot be considered valid
///                            (including the EFH's error container).  In
///                            particular, ENOSPC indicates that the EFH could
///                            not be allocated. 
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_File::Open (STFS_ExternalFileMetadata *pp_efm, 
		 int pv_openFlags,
		 STFS_ExternalFileHandle **pp_efh) {
    /// logically open the newly created file

    STFS_ExternalFileHandle *lp_Efh = pp_efm->Open(pv_openFlags);
    if (!lp_Efh) {
      errno = ENOSPC;
      return errno;
    }

    *pp_efh = lp_Efh;

  return 0;
}


///////////////////////////////////////////////////////////////////////////////
///
//          ReserveName
///
/// \brief  Reserves an STFS file name on this system
///
/// This method reserves an STFS file name on a system, blocking other creates
/// using exactly the same name.
///  
/// The current implementation reserves the name by creating a physical
/// directory on the appropriate file system.  All fragments for this external
/// file are created in this directory.
///
/// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
///
/// This assumes only one file system.  We need to adjust this code later for
/// all STFS file systems...
///
/// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
///
/// \param pp_fileName       The name to be reserved
/// 
/// \retval RetVal < 0   Name is not reserved
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_File::ReserveName (char * pv_fileName) {


  // need to get the STFS directory names for this node so we can reserve it
  // there.

  //  for now, we've just got one file system on one node, so we'll just make a
  //  simple call.  Later when we support extended storage on a node, we'll need
  //  to loop over all file systems.


  char lv_fileName[STFS_DIRNAME_MAX + STFS_NAME_MAX];

  bool lv_gotMountPoint = false;
  lv_gotMountPoint =  STFS_util::GetMountPointName (lv_fileName,NULL); 

  ASSERT (lv_gotMountPoint == true);

  char *lv_copyPtr = lv_fileName + strlen (lv_fileName);
  strcpy (lv_copyPtr, pv_fileName);


  int lv_Ret = mkdir (lv_fileName,  S_IRUSR | S_IWUSR | S_IXUSR);
  if (lv_Ret < 0) {
    if (errno != EEXIST) {

      // TBD err handling -- in particular, losing contact with the FS isn't a
      // fatal error.

      ASSERT (false);
      return -1;
    }
  }

  return 0;
}


///////////////////////////////////////////////////////////////////////////////
///
//          ReleaseName
///
/// \brief  Frees an STFS file name on this system
///
/// This method releases a reservedSTFS file name on a system, allowing other
/// creates to use the same name.
///  
/// The current implementation reserves the name by creating a physical
/// directory on the appropriate file system.  This method issues the
/// corresponding rmdir.  It presumes that the directory is empty, but never
/// checks.  It also doesn't check to see if the name is currently reserved, or
/// if the directory is even accessible.  In fact, most all errors from rmdir
/// are simply ignored.  This seems to be the best course.  If things go too
/// crazy, STFS utilities can be used to clean up the mess.
///
/// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
///
/// This assumes only one file system.  We need to adjust this code later for
/// all STFS file systems on the node...
///
/// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
///
/// \param pp_fileName       The name to be released
/// 
/// \retval void
///
///////////////////////////////////////////////////////////////////////////////
void
STFS_File::ReleaseName (char *pv_fileName) {

  char lv_fileName[STFS_DIRNAME_MAX + STFS_NAME_MAX];

  bool lv_gotMountPoint = false;

  lv_gotMountPoint =  STFS_util::GetMountPointName (lv_fileName, NULL); 
  ASSERT (lv_gotMountPoint == true);

  char *lv_copyPtr = lv_fileName + strlen (lv_fileName);
  strcpy (lv_copyPtr, pv_fileName);


  int lv_Ret = rmdir (lv_fileName);
  if (lv_Ret < 0) {
    if (errno != EEXIST) {

      // TBD err handling -- in particular, losing contact with the FS isn't a
      // fatal error.

      ASSERT (false);
    }
  }
}
