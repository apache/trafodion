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
#ifndef _INT_TYPE_H_
#define _INT_TYPE_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         IntType.h
 * Description:  Integer types used in the adaptor functions to work with
 *               NSJ 3.0 IEEE float support
 *               
 *               
 * Created:      8/15/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
// 64-bit: get Int32 and Int64 from Platform.h and Int64.h
#include "Platform.h"
#include "Int64.h"
#typedef Int64 Int64;
#typedef Lng32 Int32;
#endif
