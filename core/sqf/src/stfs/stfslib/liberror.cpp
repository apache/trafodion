///////////////////////////////////////////////////////////////////////////////
// 
/// \file    liberror.cpp
/// \brief   STFS_error implementation
///   
/// This file contains the implementation of the STFS_error() function,
/// starting with the functional STFSLIB_error function and drilling
/// down to supporting functions.
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
#include "stfsd.h"
#include "stfs_error.h"

namespace STFS {
  ///////////////////////////////////////////////////////////////////////////////
  ///
  //          STSFSLIB_error()
  ///
  ///  \brief The actual implementation of an STFS_write
  ///
  ///  This method is the driver for an STFS_write() function call.
  ///  it also implements the protocol for determining whether or not
  ///  to create a new fragment.  Only STFSLIB can make this call, not
  ///  STFS-d, so the logic is put here.  We could put a lot of "if
  ///  !ExecutingInDaemon" escape clauses, but there's no need to put
  ///  those in the mainline remote write path.  
  ///  
  ///
  /// \param  pv_Offset  byte offset count based on whence
  /// \param  pv_Whence  values are as follow: \n
  ///                         SEEK_SET - The offset is set to offset bytes. \n
  ///                         SEEK_CUR - The offset is set to its current
  ///                                    location plus offset bytes. \n
  ///                         SEEK_END - The offset is set to the size of the 
  ///                                    file plus offset bytes. 
  /// 
  /// \retval off_t SUCCESS: The resulting offset location from beginning
  ///                        of current fragment file.\n
  ///               FAILURE: -1 
  ///
  ///////////////////////////////////////////////////////////////////////////////
  int
  STFSLIB_error ( stfs_fhndl_t     pv_Fhandle,
		  int             *pp_error,
		  int             *pp_addlError, 
		  char            *pp_context,
		  size_t           pv_contextMaxLen, 
		  size_t          *pp_contextLen )  {

    const char     *WHERE = "STFSLIB_error";
    STFS_ScopeTrace lv_st(WHERE);

    //////////////////////////////////
    /// Validate parameters
    //////////////////////////////////

    if (pv_Fhandle == 0 || 
	pv_Fhandle == -1 ) {
      // Can't rollerskate without wheels, can't get the error without a handle...
      return -1;
    }

    if (pv_contextMaxLen > 0 && pp_context == NULL) {
      // said the buffer had a non-zero length, but didn't pass it
      return -1;
    }

    if (pp_context != NULL && pp_contextLen == NULL) {
      // we need to tell you how long the buffer is...
      return -1;
    }

    //////////////////////////////////
    /// Get efh
    //////////////////////////////////

    STFS_ExternalFileHandle *lp_Efh
                     = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);

    //////////////////////////////////
    /// Extract error from efh
    //////////////////////////////////
    bool lv_RetBool = lp_Efh->GetError ( pp_error,
					 pp_addlError,
					 pp_context,
					 pv_contextMaxLen,
					 pp_contextLen);

    if (lv_RetBool == false) {
      // no more errors found
      return -1;
    }

    return 0;
  }

} //namespace STFS
