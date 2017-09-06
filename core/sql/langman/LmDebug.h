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
#ifndef _LM_DEBUG_H_
#define _LM_DEBUG_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         LmDebug.h
 * Description:  debug functions of the language manager
 *               
 *               
 * Created:      6/20/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "NABoolean.h"

#ifdef LM_DEBUG
#include "NABoolean.h"
NABoolean doLmDebug();
void lmDebug(const char *, ...);
#define LM_DEBUG0(msg)                lmDebug((msg))
#define LM_DEBUG1(msg,a1)             lmDebug((msg),(a1))
#define LM_DEBUG2(msg,a1,a2)          lmDebug((msg),(a1),(a2))
#define LM_DEBUG3(msg,a1,a2,a3)       lmDebug((msg),(a1),(a2),(a3))
#define LM_DEBUG4(msg,a1,a2,a3,a4)    lmDebug((msg),(a1),(a2),(a3),(a4))
#define LM_DEBUG5(msg,a1,a2,a3,a4,a5) lmDebug((msg),(a1),(a2),(a3),(a4),(a5))
#else
#define LM_DEBUG0(msg)
#define LM_DEBUG1(msg,a1)
#define LM_DEBUG2(msg,a1,a2)
#define LM_DEBUG3(msg,a1,a2,a3)
#define LM_DEBUG4(msg,a1,a2,a3,a4)
#define LM_DEBUG5(msg,a1,a2,a3,a4,a5)
#endif

#if defined(LM_DEBUG)
void debugLmSignalHandlers();
#define LM_DEBUG_SIGNAL_HANDLERS(msg) \
lmDebug(msg); \
debugLmSignalHandlers()
#else
#define LM_DEBUG_SIGNAL_HANDLERS(msg)
#endif

#ifdef LM_DEBUG
NABoolean doNotRestoreSignalHandlersAfterUDF();
#else
inline NABoolean doNotRestoreSignalHandlersAfterUDF() { return FALSE; }
#endif

#endif // _LM_DEBUG_H_

