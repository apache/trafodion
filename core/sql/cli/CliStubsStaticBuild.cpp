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
 * File:         CliStubs.cpp
 * Description:  CLI Stubs.
 *               
 *               
 * Created:      3/22/2002
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "NAType.h"
#include "CharType.h"
//ss_cc_change : unused function
#ifndef NA_CMPDLL
//LCOV_EXCL_START
NABoolean NAType::isComparable(const NAType &other,
                               ItemExpr *parentOp,
                               Int32 emitErr) const
{ return FALSE; }
//LCOV_EXCL_STOP

NABoolean CharType::isComparable(const NAType &other,
                               ItemExpr *parentOp,
                               Int32 emitErr) const
{ return FALSE; }
#endif  // Not NA_CMPDLL








