#ifndef _STFS_FRAGMENT_H
#define _STFS_FRAGMENT_H
///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_fragment.h
///  \brief   Definition of STFS_Fragment Util Class
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

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

// For EV Log
#include "sqevlog/evl_sqlog_writer.h"
#include "common/evl_sqlog_eventnum.h"

#include "stfs_defs.h"
#include "stfs_metadata.h"

namespace STFS {
class STFS_ExternalFileHandle;
class STFS_ExternalFileMetadata;
class STFS_FragmentFileMetadata;
struct STFS_OpenersSet;
}

using namespace STFS;

namespace STFS {
///////////////////////////////////////////////////////////////////////////////
///
/// \class  STFS_Fragment
///
/// \brief  STFS Fragmentclass
///
/// This class contains utility methods to implement the UML fragment-related
/// logical fragment primitives.  These include create fragment, open fragment,
/// unlink fragment, etc.  At the end of successful calls to these methods, the
/// metadata is left in a state that is consistent with the purpose of the
/// routine (alocated and read for use at the end of create, deallocated at
/// close, EFH's allocated after an open, etc.)  These fragment primitives don't
/// always do physical operations however; in particular opens occur on demand,
/// not at calls to Open Fragment.  
///
/// All the methods/data members in this class are static.
///
///////////////////////////////////////////////////////////////////////////////
class STFS_fragment {
 public:

  static int Open (STFS_ExternalFileHandle *pp_efh,
		   int pv_fragNum,
		   STFS_fs *ppFs);

  static int InitFrag (STFS_ExternalFileMetadata *pp_Efm,
		       char *pp_FragmentFileName,
		       char *pp_StfsDirectory,
		       size_t pv_FragmentFileOffset );

  static int OpenNoReposition(void);

  static int GetStorageLocation (char *pp_stgLocation,
				 bool *pv_stgIsLocal);

  static int SetNewCurrentOpen(void);   /// Is this somewhere else already?
  static int GetCurrentOpen(void);
  static int read();
  static int write();
  static int CloseAll (void);
  static int Close(void);
  static int SetCurrentOpenToEOFRelativeAddress(void);



};

}

#endif
