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
#ifndef _UDRSERV_H_
#define _UDRSERV_H_

/* -*-C++-*-
******************************************************************************
*
* File:         udrserv.h
* Description:  Definitions for mainline code.
*
* Created:      4/10/2001
* Language:     C++
*
*
*
*
******************************************************************************
*/

#define NA_COMPILE_INSTANTIATE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "Int64.h"

#include "spinfo.h"
#include "copyright.h"
#include "udrutil.h"
#include "LmError.h"
#include "NumericType.h"
#include "udrdefs.h"
#include "udrglobals.h"

#include "logmxevent.h"

#include "UdrExeIpc.h"

#include "ComDiags.h"

extern UdrGlobals *UDR_GLOBALS; // UDR globals area

jmp_buf UdrHeapLongJmpTgt; // udrHeap memory failure
jmp_buf IpcHeapLongJmpTgt; // ipcHeap memory failure

#define UDRSERV_ERROR_PREFIX   "*** ERROR[] "

#endif // _UDRSERV_H_

