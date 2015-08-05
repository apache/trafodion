#ifndef STFS_ROOT_H
#define STFS_ROOT_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_root.h
///  \brief   STFS_Root class
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

#include "seabed/thread.h"

namespace STFS {

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_Root
  ///
  /// \brief  STFS Root class
  ///
  /// This is the root class for most STFS classes. Implements:
  ///    - eye catcher functionality
  ///    - Mutex
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_Root {
  public:

    enum Enum_EC {
      EC_ExternalFileMetadata = 0,
      EC_ExternalFileHandle,
      EC_FragmentFileMetadata,
      EC_FragmentFileHandle,
      EC_ExternalFileMetadataContainer,
      EC_ExternalFileHandleContainer,
      EC_ExternalFileOpenerContainer,
      EC_Session,
      EC_Error,
      EC_Unknown
    };

    typedef struct {
      Enum_EC  EyeCatcherEnum;
      char    *EyeCatcherString;
    } EyeCatcherMap_Def;
    
    static EyeCatcherMap_Def EyeCatcherMap_[];
      
    STFS_Root(Enum_EC pv_EyeCatcherEnum);

    virtual ~STFS_Root() {};
    
    char * EyeCatcherGet() { 
      return eyeCatcher_;
    }

    static char *GetEyeCatcherString(Enum_EC pv_EyeCatcherEnum);
    virtual bool IsEyeCatcherValid() {return true;};
    
  protected:
    bool IsEyeCatcherValid(Enum_EC pv_EyeCatcherEnum);
    size_t Pack (char *pp_buf, size_t pv_spaceRemaining);
    size_t Unpack (char *pp_buf);
    int Lock();
    int Unlock();
    SB_Thread::Mutex mutex_;

  private:
    STFS_Root() {}
    char eyeCatcher_[4];
  };

}

#endif
