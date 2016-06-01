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
#ifndef _LM_EXT_FUNC_H_
#define _LM_EXT_FUNC_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         LmExtFunc.h
 * Description:  Functins needed by the language manager
 *               
 *               
 * Created:      5/4/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"
#include "NABoolean.h"

extern void lmMakeTFDSCall(const char *msg, const char *file, UInt32 line);
extern void lmPrintSignalHandlers();
extern NABoolean lmSetSignalHandlersToDefault();
extern NABoolean lmRestoreJavaSignalHandlers();
extern NABoolean lmRestoreUdrTrapSignalHandlers(NABoolean saveJavaSignalHandlers);

#endif



