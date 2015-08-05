#ifndef STFS_LIBLSEEK_H
#define STFS_LIBLSEEK_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    liblseek.h
///  \brief   Header file for STFSLIB_lseek-associated functionality
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

/////////////////////////////////////
/// function protototypes
/////////////////////////////////////
namespace STFS {
  ssize_t STFSLIB_lseek( stfs_fhndl_t pvFhandle, off_t pv_Offset,
			 int pv_Whence);

} // Namespace STFS

#endif // STFS_LIBLSEEK_H
