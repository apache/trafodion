///////////////////////////////////////////////////////////////////////////////
// 
/// \file    libselect.cpp
/// \brief   STFS_select implementation
///   
/// This file contains the implementation of the STFS_select() function,
/// starting with the functional STFSLIB_select function and drilling
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

#include "libselect.h"

namespace STFS {


  int ProcessFdSet(nfhs_t  pv_Nfhs, 
		   nfhs_t *pp_Nfds,
		   fh_set *pp_Fhs,
		   fd_set *pp_Fds)        
                    
  {
    nfhs_t lv_Fd = 0;
    if(pp_Fhs != NULL) {
      FD_ZERO(pp_Fds);
      for (int i = 0; i < pv_Nfhs; i++) {
	if(i < (int)pp_Fhs->FhArray.size()) {
	  STFS_ExternalFileHandle *lp_Efh = STFS_ExternalFileHandle::GetExternalFileHandle(pp_Fhs->FhArray[i]);
	  if (! lp_Efh) {
	    TRACE_PRINTF2(1, "Error in obtaining External File Handle for %ld\n", pp_Fhs->FhArray[i]);
	    return -1;
	  }
	  //Check for local file.  
	  //Check for Nonblocking or not if so ignore.    
	  if(((lp_Efh->OpenFlagsGet())&O_NONBLOCK) == O_NONBLOCK) {
	    lv_Fd = lp_Efh->GetFD();
	    if(lv_Fd >= 0) {
	      FD_SET(lv_Fd, pp_Fds);
	      if(lv_Fd > *pp_Nfds) {
		*pp_Nfds = lv_Fd; 
	      }
	    }
	  }
	}
      }
    }
  
    return 0;
  }

  int ProcessFhSet(nfhs_t  pv_Nfhs, 
		   fh_set *pp_Fhs, 
		   fd_set *pp_Fds) 
  {
    int lv_Fd = 0;
    struct fh_set lv_Fhs;

    if(pp_Fhs != NULL) {
      lv_Fhs.FhArray = pp_Fhs->FhArray; 
      STFS_FH_ZERO(pp_Fhs);
      for (int i = 0; i < pv_Nfhs; i++) {
	if(i < (int)lv_Fhs.FhArray.size()) {
      
	  STFS_ExternalFileHandle *lp_Efh = STFS_ExternalFileHandle::GetExternalFileHandle(lv_Fhs.FhArray[i]);
	  if (! lp_Efh) {
	    TRACE_PRINTF2(1, "Error in obtaining External File Handle for %ld\n", lv_Fhs.FhArray[i]);
	    return -1;
	  }
	  if(((lp_Efh->OpenFlagsGet())&O_NONBLOCK) == O_NONBLOCK) {
	    lv_Fd = lp_Efh->GetFD();
	    if(lv_Fd >= 0) {
	      if(FD_ISSET(lv_Fd, pp_Fds)) {
		//Insert into fh_set
		pp_Fhs->FhArray.push_back(pp_Fhs->FhArray[i]);
	      }
	    }
	  }
	}
      }
    }
    return 0;
  }
  int STFSLIB_select( stfs_fhndl_t    pv_Nfhs,        
		      fh_set         *pp_ReadFhs, 
		      fh_set         *pp_WriteFhs, 
		      fh_set         *pp_ExceptFhs, 
		      struct timeval *pp_Timeout)
  {
    nfhs_t lv_Nfds = 0;
    fd_set *lp_ReadFds = new fd_set(),
      *lp_WriteFds = new fd_set(), 
      *lp_ExceptFds = new fd_set();

    if(pv_Nfhs >= FH_ARRAY_SIZE) { 
      return -1;
    }
    STFS::ProcessFdSet(pv_Nfhs, &lv_Nfds, pp_ReadFhs, lp_ReadFds);
    STFS::ProcessFdSet(pv_Nfhs, &lv_Nfds, pp_WriteFhs, lp_WriteFds);
    STFS::ProcessFdSet(pv_Nfhs, &lv_Nfds, pp_ExceptFhs, lp_ExceptFds);

    lv_Nfds++;
    int lv_RetVal = select(lv_Nfds, lp_ReadFds, lp_WriteFds, lp_ExceptFds, pp_Timeout);
    if(lv_RetVal < 0) {
      //TODO: ERROR Tracing
      return -1;
    }

    STFS::ProcessFhSet(pv_Nfhs, pp_ReadFhs, lp_ReadFds);
    STFS::ProcessFhSet(pv_Nfhs, pp_WriteFhs, lp_WriteFds);
    STFS::ProcessFhSet(pv_Nfhs, pp_ExceptFhs, lp_ExceptFds);

    //Set as passed for now
    return lv_RetVal;
  }

} //namespace STFS
