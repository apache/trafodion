//------------------------------------------------------------------
//
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

#ifndef __SB_THREADTLSX_H_
#define __SB_THREADTLSX_H_

#include "seabed/thread.h"

extern void SB_add_tls_key(int pv_key, const char *pp_desc);
extern int  SB_create_tls_key(SB_Thread::Sthr::Dtor_Function  pp_ptor,
                              const char                     *pp_desc);

#endif // __SB_THREADTLSX_H_
