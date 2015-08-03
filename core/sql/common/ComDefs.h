#ifndef COMDEFS_H
#define COMDEFS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComDefs.h
 * Description:  Common defines used by both the optimizer and executor.
 *               
 * Created:      7/06/99
 * Language:     C++
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
 *****************************************************************************
 */

///////////////////////////////////////////////////////////////////
// This file is included by executor, do not add any includes in
// here to make it executor-noncompliant.
///////////////////////////////////////////////////////////////////
#define ROUND2(size) \
   Long(((size) + 1) & (~1))

#define ROUND4(size) \
   Long(((size) + 3) & (~3))

#define ROUND8(size) \
   Long(((size) + 7) & (~7))

#define ROUND16(size) \
   Long(((size) + 15) & (~15))

#define ROUNDto1KinB(size) \
   Long(((size-1)/1024 + 1) * 1024)

#define ROUNDto1KinK(size) \
   Long(ROUNDto1KinB(size) / 1024)

#define ROUNDto4KinB(size) \
   Long(((size-1)/4096 + 1) * 4096)

#define ROUNDto4KinK(size) \
   Long(ROUNDto4KinB(size) / 1024)


#endif  // COMDEFS_H

