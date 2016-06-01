#ifndef NAEXIT_H
#define NAEXIT_H
/* -*-C++-*-
**************************************************************************
*
* File:         NAExit.h
* Description:  Simple error functions that should be (globally) common
* Created:      7/18/1998
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
**************************************************************************
*/

// DO NOT ADD ANY DEPENDENCIES TO THIS FILE!!!
//
// THIS FILE IS INTENDED TO BE GLOBALLY USEABLE/USEFUL, AND THE ONLY WAY
// THAT WILL WORK IS IF IT'S SAFE FOR ANYONE TO INCLUDE IT!

// NAExit() is just a simple wrapper around exit() which calls
// NAError_stub_for_breakpoints() (in NAError.h/BaseTypes.cpp) before
// quitting the process completely

void NAExit (Int32 status);  // fn body is in BaseTypes.cpp


#endif /* NAEXIT_H */


