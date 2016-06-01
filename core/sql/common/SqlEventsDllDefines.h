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

#ifndef SQLEVENTS_DLL_DEFINES_H
#define SQLEVENTS_DLL_DEFINES_H

#include "Platform.h"

  #ifndef SQLEVENTS_LIB_FUNC
    #ifdef  __EID
      #define SQLEVENTS_LIB_FUNC
    #else
      #ifdef  EVENTS_DLL
        #define SQLEVENTS_LIB_FUNC __declspec( dllexport )
      #else
        #define SQLEVENTS_LIB_FUNC __declspec( dllimport )
      #endif
    #endif  // __EID
  #endif  // SQLEVENTS_LIB_FUNC
#ifdef NA_64BIT
  // dg64 - get rid of __declspec
  #undef SQLEVENTS_LIB_FUNC
  #define SQLEVENTS_LIB_FUNC
#endif

#endif  // SQLEVENTS_DLL_DEFINES.h

