/**********************************************************************
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
**********************************************************************/

#ifndef SQLEXPDLL_H
#define SQLEXPDLL_H

#include "Platform.h"

  #ifdef __EID
    #define SQLEXP_LIB_FUNC
  #else
    #ifdef EXP_DLL
      #define SQLEXP_LIB_FUNC __declspec( dllexport )
    #else
      #ifdef UDRSERV_BUILD
        #define SQLEXP_LIB_FUNC
      #else
        #define SQLEXP_LIB_FUNC __declspec( dllimport )
      #endif // UDRSERV_BUILD
    #endif // EXP_DLL
  #endif // __EID
#ifdef NA_64BIT
  // dg64 - get rid of __declspec
  #undef SQLEXP_LIB_FUNC
  #define SQLEXP_LIB_FUNC
#endif

#endif // SQLEXPDLL_H


