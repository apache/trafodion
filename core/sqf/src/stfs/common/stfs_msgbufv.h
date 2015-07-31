#ifndef STFS_MSGBUFV_H
#define STFS_MSGBUFV_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_message.h
///  \brief   STFSMessage_* template classes for interprocess comm.
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


#include "stfs/stfslib.h"
#include "stfs_root.h"
#include "stfs_error.h"
#include "stfs_message.h"
#include "stfs_metadata.h"
#include "stfs_util.h"
#include "seabed/thread.h"

namespace STFS {


#define STFSMSGBUFV_VARIABLE_EFMMAX  16384


#define STFSMSGBUFV_VARIABLE_ERRMAX  16384
  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBufV_PackedMetadata
  /// 
  /// \brief Encapsulation for packing EFM and associated FFMs
  ///
  /// This class encapsulates the metadata packing for STFS
  /// interprocess messages
  ///
  ///  The variable-sized data buffer for EFM information looks like
  ///  this: 
  ///
  ///    *-----------------*
  ///    *                 *
  ///    *   Packed EFM    *     The EFM packed using the EFM->pack() method
  ///    *                 *
  ///    *-----------------*
  ///    *                 *
  ///    *   FFM Offset    *
  ///    *      Array      *
  ///    *                 *
  ///    *-----------------*
  ///    *                 *
  ///    *   First packed  *
  ///    *      FFM        *
  ///    *                 *
  ///    *-----------------*
  ///    *                 *
  ///    *   Second packed *
  ///    *      FFM        *
  ///    *                 *
  ///    *-----------------*
  ///    *                 *
  ///    *     ....        *
  ///    *                 *
  ///    *-----------------*
  ///
  ///////////////////////////////////////////////////////////////////////////////

/*
  class STFSMsgBufV_PackedMetadata {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////
    STFSMsgBufV_PackedMetadata();
    STFSMsgBufV_PackedMetadata ( STFS_ExternalFileMetadata *pp_EFM  );

    ////////////////////////
    /// Destructors
    ////////////////////////

    ~STFSMsgBufV_PackedMetadata();

    ////////////////////////
    ///  Get Methods
    ////////////////////////
    STFS_ExternalFileMetadata STFSMsgBufV_ExtractMetadata ();


    ////////////////////////
    /// Set Methods
    ////////////////////////
    void SetExtractionStartPoint (char *pp_extractionPoint);

  private:
    varoffset_t  packedEFMOffset_;
    varoffset_t *FFMOffsetArray_[];   // Points to the FFM offset
                                      // array, number of entries is
				      // the number of FFMs
    char *currentObjectToPack_;        // pointer for the current
				      // object we're packing, EFM or FFM

  }
   */
} // namespace

 


#endif // ifndef STFS_MSGBUFV_H
