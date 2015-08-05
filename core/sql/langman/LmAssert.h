#ifndef _LM_ASSERT_H_
#define _LM_ASSERT_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         LmAssert.h
 * Description:  assertion function of the language manager
 *               
 *               
 * Created:      5/4/02
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
 *****************************************************************************
 */
#include <iostream>
#include "Platform.h"

#define LM_ASSERT(p) \
  if (!(p)) { lmAssert(__FILE__, __LINE__, ""); }

#define LM_ASSERT1(p, m) \
  if (!(p)) { lmAssert(__FILE__, __LINE__, (m)); }

void lmAssert(const char *file, Int32 linenum, const char *msg="");


#endif



