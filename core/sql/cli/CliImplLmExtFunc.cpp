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
 * File:         CliImplLmExtFunc.cpp
 * Description:  Functions needed by the language manager
 *               (derived from ../udrserv/UdrImplLmExtFunc.cpp)
 *               
 * Created:      4/9/2015
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"
#include "LmExtFunc.h"

void lmMakeTFDSCall(const char *msg, const char *file, UInt32 line)
{
  // makeTFDSCall(msg, file, line);
  abort();
}


// LCOV_EXCL_START
void lmPrintSignalHandlers()
{
  // printSignalHandlers();
}
// LCOV_EXCL_STOP

NABoolean lmSetSignalHandlersToDefault()
{
  // return setSignalHandlersToDefault();
  return TRUE;
}

NABoolean lmRestoreJavaSignalHandlers()
{
  // return restoreJavaSignalHandlers();
  return TRUE;
}

NABoolean lmRestoreUdrTrapSignalHandlers(NABoolean saveJavaSignalHandlers)
{
  // return restoreUdrTrapSignalHandlers(saveJavaSignalHandlers); 
  return TRUE;
}
