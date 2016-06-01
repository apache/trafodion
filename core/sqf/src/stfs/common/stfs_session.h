#ifndef STFS_SESSION_H
#define STFS_SESSION_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_session.h
///  \brief   Header file for the session class
///
///  The session class will hold an STFS_error variable for a specific session. 
///  The session will be for the api calls. 
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

#include "stfs_root.h"
#include "stfs_error.h"

namespace STFS {

  /////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFS_Session
  ///
  /// \brief STFS_Session class
  ///
  /// This class will hold the STFS_error variable and methods to get, 
  /// set, and check it.    
  ///
  /////////////////////////////////////////////////////////////////////////////
  class STFS_Session : public STFS_Root
  {
  public:
    virtual bool IsEyeCatcherValid();

    static STFS_Session * session_;

    static STFS_Session* GetSession();

    bool GetError ( int    *pp_errNo,
                    int    *pp_additionErr,
                    char   *pp_context,
                    size_t  pv_contextLenMax,
                    size_t *pp_contextLenOut
                    );

    bool HasError ();

    bool SetError ( bool   pv_errorIsHighest,
                    int    pv_errNo,
                    int    pv_addlErr,
                    char  *pp_contextBuf,
                    size_t pv_contextBufLen);

    void ResetErrors ();

  private:
    //To enforce a singleton container
    STFS_Session();

    ~STFS_Session();

    STFS_Error                        sessionError_;
  };
} //namespace STFS;

#endif // STFS_SESSION_H
