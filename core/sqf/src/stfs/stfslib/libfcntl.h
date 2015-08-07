#ifndef STFS_LIBFCNTL_H
#define STFS_LIBFCNTL_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    libclose.h
///  \brief   Header file for STFSLIB_Close-associated functionality
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

  int STFSLIB_fcntl (stfs_fhndl_t pv_Fhandle,  int pv_Cmd);
  int STFSLIB_fcntl (stfs_fhndl_t pv_Fhandle, int pv_Cmd, long pv_Flag);
  

} // namespace STFS

#endif //ifndef STFS_LIBCLOSE_H
