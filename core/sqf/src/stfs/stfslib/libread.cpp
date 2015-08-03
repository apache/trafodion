///////////////////////////////////////////////////////////////////////////////
// 
/// \file    libread.cpp
/// \brief   STFS_read implementation
///   
/// This file contains the implementation of the STFS_read() function,
/// starting with the functional STFSLIB_read function and drilling
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

  //////////////////////////////////////////////////////////////////////////////
  ///
  //              LibRead_GetMoreFragments
  ///
  /// \brief  Obtains updated EFM data to determine if there are additional
  ///         fragments 
  ///
  /// This function updates an existing EFM if needed.  The latest file metadata
  /// information is requested from the owning STFSd process.  When it is
  /// received, if the number of fragments doesn't match the number of fragments
  /// in the "old" EFM, then the new fragments are added to the list
  ///
  /// \param  pp_Efh   The handle associated with this operation.
  ///
  /// \retval true     New fragments were detected
  /// \retval false    No new fragments were detected or communications with
  ///                  STFSd were not satisfactory.
  ///
  //////////////////////////////////////////////////////////////////////////////

  bool 
  LibRead_GetMoreFragments (STFS_ExternalFileHandle *pp_Efh) {
    const char       *WHERE = "LibRead_GetMoreFragments ";
    STFS_ScopeTrace   lv_st(WHERE,2);

    /////////////////////////////
    ///    Check Parameters
    ////////////////////////////
    if (pp_Efh == NULL) {
      ASSERT (false);
      return false;
    }

    //Will the order of the ffh's match in the vector between daemon and lib?
    //How can we tell which one to put... Can add logic to compare
    
    //int lv_Ret = 0;
    //For now just work on grabbing the efm from the daemon

    /////////////////////////////
    /// Setup and Housekeeping
    /////////////////////////////
    bool lv_newFragmentsFound = false;

    /////////////////////////////
    /// Finis!
    /////////////////////////////

    return lv_newFragmentsFound;
  }


  //////////////////////////////////////////////////////////////////////////////
  ///
  //              STFSLIB_read
  ///
  /// \brief  Driver code for an STFS_read() library call
  ///
  //////////////////////////////////////////////////////////////////////////////
  ssize_t 
  STFSLIB_read( stfs_fhndl_t  pv_Fhandle,
                void         *pp_Buf,
                size_t        pv_Count)
  {
    const char     *WHERE = "STFSLIB_read";
    STFS_ScopeTrace lv_st(WHERE);

    if (!pp_Buf) {
      errno = EFAULT;
      return -1;
    }


    STFS_ExternalFileHandle *lp_Efh = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
    if (! lp_Efh) {
      TRACE_PRINTF2(1, "Error in obtaining External File Handle for %ld\n", pv_Fhandle);
      return -1;
    }

    lp_Efh->ResetErrors();


    size_t lv_totalBytesRead = 0;
    bool lv_readCompleted = false;

    char * lp_currBufPtr = (char *) pp_Buf;
    size_t lv_readSize = pv_Count;

    while (!lv_readCompleted) {

      ssize_t lv_Stat = 0;
      int     lv_Ret  = 0;

      //Check if the current fragment is local.  If not, then send req to daemon
      lv_Ret =  lp_Efh->IsCurrFragmentLocal();
      if(lv_Ret < 0) {
        //error evaluating fragment
        return -1;
      }   
      else if(lv_Ret == 1) {
        lv_Stat = lp_Efh->Read((char *)lp_currBufPtr,
                                       lv_readSize);
      }
      else {
#ifdef SQ_PACK
        lv_Stat= SendToSTFSd_read();
#else
        lv_Stat = STFSd_read(&(lp_Efh->OpenIdentifierGet()),
                             (char *)lp_currBufPtr,
                             lv_readSize);
#endif
        //The currentFFH_ is not local so we will need to read 
        //  through remote daemon
      }

      if (( (size_t)lv_Stat + lv_totalBytesRead) == pv_Count) {

        // the usual case; we read everything as expected.  Prepare to exit our
        // read-loop 

        lv_totalBytesRead += lv_Stat;
        lv_readCompleted = true;

      }
      else if (lv_Stat == 0) {

        // Didn't read anything else in this fragment.  Can we roll to another?

        // Gotcha!  There might be a new fragment here!  Check to see if there
        // are any new fragments then loop around and read again.

        // If we're moving to a new FFH that's already in our list, the
        // reposition at the top of the loop should move to it automatically,
        // so we just skip ahead.
        //
        // But if this is the last FFH, we need to see if either this process
        // or the owning STFSd knows about any more fragments.  We take a lazy
        // approach to doing this, hoping that somewhere along the way,
        // someone has already retrieved this information for us:  
        //
        //   1) See if the EFM already knows about other fragments. Occurs
        //      this process owns read access to the file in another open, or
        //      another opener in this process already got an updated list of
        //      FFMs.
        //
        //   2) If the number of FFHs matches the number of FFMs and we're in
        //      the last FFM, check with the owning daemon to update the
        //      metadata for this file.


        if (lp_Efh->CurrentFFHIsLast() == true) {

          if (lp_Efh->NewFFHsNeeded() == true) {

            // Someone else found fragments for us.  Logically open them for
            // this efh and try the read again.

            lv_Stat = lp_Efh ->OpenNewFragments();
            if (lv_Stat < 0) {
              return (lv_totalBytesRead>0 ? lv_totalBytesRead : lv_Stat);
            }
          }

          else {

            // We have all the FFHs that this lib process knows about,
            // check to see if new fragments available

            bool lv_newFragsDetected = LibRead_GetMoreFragments (lp_Efh);
              
            if (lv_newFragsDetected == false) {
              // a real EOF! Clean up errno after all the calls we've made

              errno = 0;
              return lv_totalBytesRead;
            }

            // We found new fragments.  Open them and try to read the rest of
            // the data.

            lv_Stat = lp_Efh ->OpenNewFragments();

            if (lv_Stat < 0) {
              // hope that any error was set below
              return (lv_totalBytesRead>0 ? lv_totalBytesRead : -1);
            }
          } // else
        }

      } // 0 count error handling

      else if (lv_Stat<0) {

        // Error!  We might be done here.  We can't just return this error
        // though.  We have to check to see if we read anything.  If so, return
        // that value and when the caller comes back, the error might (or might
        // not) happen again.
          
        return (lv_totalBytesRead>0 ? lv_totalBytesRead : -1);

      }

      else {

        // partial read!  Adjust our counters and try again
        // find out...

        ASSERT ( (size_t) lv_Stat < pv_Count);

        lv_totalBytesRead += lv_Stat;
        lp_currBufPtr = (char *)lp_currBufPtr + lv_Stat;
        lv_readSize -= lv_Stat;

      }

      if (lp_Efh->SetCurrentFragmentBasedOnCurrOffset() == true) {
        // switched frags!  We're reading sequentially, so we need to do an
        // explicit lseek in case this fragment's FD was previously used for
        // some other I/O operation.

        lv_Stat = lp_Efh->SeekToCurrFragmentStart();

        if (lv_Stat < 0) {
          // physical position failed!
          return (lv_totalBytesRead>0 ? lv_totalBytesRead : lv_Stat);
        }
      }

    } //while !readcompleted

    return lv_totalBytesRead;
  }

} //namespace STFS
