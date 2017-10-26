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
#ifndef CLI_SEMAPHORE_H
#define CLI_SEMAPHORE_H 1

/* -*-C++-*-
******************************************************************************
*
* File:         CLISemaphore.h
* Description:  Header file containing CLI Semaphore implementation.
*               
* Created:      7/21/97
* Language:     C++
*
*
*
******************************************************************************
*/



#ifndef NA_NO_GLOBAL_EXE_VARS
#ifndef CLI_GLOBALS_DEF_
#include "Globals.h"
class CLISemaphore;

extern CliGlobals *cli_globals;
extern CLISemaphore *getCliSemaphore();
#endif
#endif

// define the semaphore functions and mechanism
class CLISemaphore
{
private:
   CRITICAL_SECTION cs;
public:
   void  get();
   void  release();
   CLISemaphore();
  ~CLISemaphore();
};

inline void CLISemaphore::get()
{
   EnterCriticalSection(&cs);

}

inline void CLISemaphore::release()
{

   LeaveCriticalSection(&cs);
}

inline CLISemaphore::CLISemaphore()
{
   InitializeCriticalSection(&cs);
}

inline CLISemaphore::~CLISemaphore()
{
   DeleteCriticalSection(&cs);
}

extern CLISemaphore globalSemaphore;

#endif
