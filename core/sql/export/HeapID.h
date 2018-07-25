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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         HeapID.h
 * Description:  This file contains the declaration of HeapID class to 
 *               associate heap and heaplog.
 *               
 * Created:      3/1/99
 * Language:     C++
 *
 *
*
****************************************************************************
*/

#ifndef HEAPID__H
#define HEAPID__H

#include "Platform.h"

// -----------------------------------------------------------------------
// NA_DEBUG_HEAPLOG indicates this is a debug build on NT and 
// heaplog is importable.  However, it can be enabled for NSK
// platform later.
// -----------------------------------------------------------------------
#ifndef NA_DEBUG_HEAPLOG
#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__NOIMPORT_HEAPL)
 #define NA_DEBUG_HEAPLOG
#endif
#endif

// -----------------------------------------------------------------------
#ifdef NA_DEBUG_HEAPLOG
// -----------------------------------------------------------------------

class HeapID
{
public:

  HeapID();

  ~HeapID();

// -----------------------------------------------------------------------
#else
// -----------------------------------------------------------------------

class HeapID
{
public:

  HeapID() : heapNum(-1) {}

  ~HeapID() {}

#endif
  Lng32 heapNum;
};

#endif


