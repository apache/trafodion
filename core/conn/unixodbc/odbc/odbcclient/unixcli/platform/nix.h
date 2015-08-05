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
********************************************************************/

#ifndef __NIX_DRIVER__
#define __NIX_DRIVER__

#include <asyncIO.h>

#ifdef __cplusplus
extern "C" {
#endif

#define InitializeCriticalSection2(LPCRITICAL_SECTION) ((void)0)
#define DeleteCriticalSection2(LPCRITICAL_SECTION) ((void)0)
int EnterCriticalSection2(Async_Mutex *mutex);
int LeaveCriticalSection2(Async_Mutex *mutex);

#define CRITICAL_SECTION2 Async_Mutex

#ifdef __cplusplus
}
#endif

#endif /* __NIX_DRIVER__ */
