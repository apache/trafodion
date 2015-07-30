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
#ifndef HSCONT_H
#define HSCONT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_cont.h
 * Description:  Context functions.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "hs_cmp.h"
#include "hs_const.h"

// -----------------------------------------------------------------------
// Externals.
// -----------------------------------------------------------------------
class HSGlobalsClass;

// -----------------------------------------------------------------------
// Functions.
// -----------------------------------------------------------------------
enum { NULL_USTAT_ID=-1 };
enum { USTAT_MAX_NUM_CONTEXTS = 5 };

UstatContextID AddHSContext(HSGlobalsClass *hs_globals);
void DeleteHSContext(UstatContextID id);
HSGlobalsClass* GetHSContext(UstatContextID id = NULL_USTAT_ID);


#endif /* HSCONT_H */





