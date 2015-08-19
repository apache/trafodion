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
 * File:         NAStdlib.h
 * Description:  Include file for std library with and without C runtime
 *               (set of functions is of course restricted w/o C runtime)
 *               
 * Created:      February 2000
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include <string.h>

// -----------------------------------------------------------------------
// The purpose of this file is to define C RUNTIME stuff found in the
// system library if NA_NO_C_RUNTIME is defined. Otherwise, the regular
// headers are included.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// NT or NSK with C runtime available, use the standard header files
// -----------------------------------------------------------------------
#include <stdlib.h>
#include <memory.h>

