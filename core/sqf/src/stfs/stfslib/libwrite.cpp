///////////////////////////////////////////////////////////////////////////////
// 
/// \file    libwrite.cpp
/// \brief   STFS_write implementation
///   
/// This file contains the implementation of the STFS_write() function,
/// starting with the functional STFSLIB_write function and drilling
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
#include "stfs_message.h"
#include "stfsd.h"

#include "send.h"

namespace STFS {
  ///////////////////////////////////////////////////////////////////////////////
  ///
  //          STSFSLIB_write()
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
  ssize_t 
  STFSLIB_write( stfs_fhndl_t  pv_Fhandle,
		 const void   *pp_Buf,
		 size_t        pv_Count)
  {
    const char     *WHERE = "STFSLIB_write";
    STFS_ScopeTrace lv_st(WHERE);

    if (!pp_Buf) {
      errno = EINVAL;
      return -1;
    }

    STFS_ExternalFileHandle *lp_Efh
                     = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
    if (! lp_Efh) {
      TRACE_PRINTF2(1, "Error in obtaining External File Handle for %ld\n", pv_Fhandle);
      return -1;
    }

    lp_Efh->ResetErrors();

    bool lv_WriteCompleted = false;
    ssize_t lv_totalCountWritten = 0;

    void *lp_currBuffPtr = (void *) pp_Buf;
    ssize_t lv_writeSize = pv_Count;

    while (!lv_WriteCompleted)
      {
	lp_Efh->SetCurrentFragmentBasedOnCurrOffset();
	      
	ssize_t lv_Stat = lp_Efh->Write( lp_currBuffPtr,
					 lv_writeSize);

	if (lv_Stat == lv_writeSize) {
	  // The usual case: we wrote everything as expected.  Prepare
	  // to exit the loop and go home for the day!

	  lv_totalCountWritten += lv_writeSize;
	  lv_WriteCompleted = true;

	}

	else if (lv_Stat < 0) {
	  // Hmm, do we need to create a new fragment?
	  
	  if (errno == ENOSPC) {
	    //gotcha!  Create a new fragment and continue writing the
	    //buffer

	    STFS_ExternalFileMetadata *lp_EFM = lp_Efh->EfmGet();

	    // make sure that the lib has the same opinion as the
	    // daemon on the new fragment's offset 

	    size_t lv_newFragExpectedOffset = lp_Efh->CurrentOffsetGet();

#ifdef SQ_PACK

	    STFS_OpenIdentifier lp_openIdentifier = lp_Efh->OpenIdentifierGet();

	    lv_Stat = SendToSTFSd_createFrag(lp_EFM,
					     STFS_util::GetMyNodeId(),
					     STFS_util::GetMyPID(),
					     &lp_openIdentifier);
#else
	    lv_Stat = STFSd_createFragment(lp_EFM,
					  STFS_util::GetMyNodeId(),
					  STFS_util::GetMyPID(),
					  lp_Efh->CurrentOffsetGet());
#endif

	    if (lv_Stat < 0) {
	      // Fragment not created.  Treat it like a normal ENOSPC
	      // error after all.  If the error on the createFrag was
	      // something else, then we'll hope that it was put in
	      // the open error structure...

	      errno = ENOSPC; 
	    }
	    else {
	      // open the new fragment

	      lv_Stat = lp_Efh->OpenNewFragments();

	      if (lv_Stat < 0) {
		errno=ENOSPC;
	      }

	      // Now check to see if the new fragment has the correct
	      // starting offset.  The new fragment isn't yet the
	      // current one, so we have to extract the current number
	      // of fragments, use it as the index to get the last
	      // fragment's FFM, and call OffsetGet on that fragment.

	      size_t lv_newFragRealStartOffset =  lp_Efh->GetLastFFMStartOffset();

	      if (lv_newFragRealStartOffset != lv_newFragExpectedOffset) {
		
		// Something's wrong because the daemon has a
		// different opinion on the previous fragment's
		// size or our calculations are out of sync. This
		// is a data integrity problem

		//  This code should really be fixed to change the
		//  if condition to be a Permassert or the
		//  following Assert (false) to kill the library
		//  process.

		STFS_util::SoftwareFailureHandler (WHERE);
	      }
	    }
	  }

	  if (lv_Stat < 0) {

	    // we've got a real, live error here!
	    // before we just return this error, check to see if we've
	    // written anything!  If so, we want to return that
	    // value.  It's up to the library caller to decide whether
	    // to retry the I/O in that case.

	    if (lv_totalCountWritten >  0) {

	      return lv_totalCountWritten;

	    }
	    else {

	      // a real error! Make sure errno wasn't overwritten by
	      // this point! Any additional information or subsequent
	      // errors should be stuffed in the error structure by
	      // lower calls.

	      return -1;

	    }

	  }  // Not ENOSPC check

	}  // lv_Stat < 0 check

	else  {

          //Rev. Following line is comparing signed vs unsigned. 
	  ASSERT ((size_t)(lv_totalCountWritten+lv_Stat) < pv_Count);

	  // partial write!  Hmmmmm, might mean we need a new fragment
	  // (or it might mean there was partial write).  We need to
	  // save the number of bytes we wrote and then try again to
	  // write the data.  We also have to save our current
	  // location in the buffer and calculate the new number of
	  // bytes to write.

	  // Careful here:  it's possible that a single large write
	  // might create more than one fragment! Make sure that
	  // whatever code gets added uses lv_CountWritten and lv_Stat
	  // with the proper context.

	  lv_totalCountWritten += lv_Stat;
	  lp_currBuffPtr = (char *)lp_currBuffPtr + lv_Stat;
	  lv_writeSize -= lv_Stat;

	}
      } // While !lv_WriteCompleted

    return lv_totalCountWritten;
  }

} //namespace STFS
