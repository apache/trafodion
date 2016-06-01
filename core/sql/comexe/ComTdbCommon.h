/* -*-C++-*-
****************************************************************************
*
* File:         ComTdbCommon.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
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
*
*
****************************************************************************
*/

#ifndef COMTDBCOMMON_H
#define COMTDBCOMMON_H

// --------------------------------------------------------------------------
// This header file includes the common header files needed by the generator
// implementations of all TDB subclasses.
// --------------------------------------------------------------------------

// These headers are needed in all implementations of TDB::pack().
#include "exp_stdh.h"        // rewrite PACK_CRI_DESC() to ex_cri_desc->pack()
#include "ExpCriDesc.h"      // for ex_cri_desc->pack()

#endif
