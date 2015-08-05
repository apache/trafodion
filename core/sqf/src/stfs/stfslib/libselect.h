#ifndef STFS_LIBSELECT_H
#define STFS_LIBSELECT_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    libselect.h
///  \brief   Header file for STFSLIB_select-associated functionality
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

  int ProcessFdSet (nfhs_t  pv_Nfhs,
		    nfhs_t *pp_Nfds,
		    fh_set *pp_Fhs,
		    fd_set *pp_Fds );

  int ProcessFhSet (nfhs_t  pv_Nfhs,
		    fh_set *pp_Fhs,
		    fd_set *pp_Fds);

  int STFSLIB_select ( stfs_fhndl_t    pv_Nfhs,
		       fh_set         *pp_ReadFhs,
		       fh_set         *pp_WriteFhs,
		       fh_set         *pp_ExceptFhs,
		       struct timeval *pp_Timeout);

} //namespace STFS

#endif //ifndef STFS_LIBSELECT_H
