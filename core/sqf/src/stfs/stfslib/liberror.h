#ifndef STFS_LIBERROR_H
#define STFS_LIBERROR_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    liberror.h
///  \brief   Header file for STFSLIB_error-associated functionality
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

  ssize_t STFSLIB_error (stfs_fhndl_t     pv_Fhandle,
			 int             *pp_error,
			 int             *pp_addlError, 
			 char            *pp_context,
			 size_t           pv_contextMaxLen, 
			 size_t          *pp_contextLen );

} //namespace STFS;

#endif // STFS_LIBERROR_H
