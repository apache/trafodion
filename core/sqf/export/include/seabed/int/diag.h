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

#ifndef __SB_INT_DIAG_H_
#define __SB_INT_DIAG_H_

#include "opts.h"

#ifdef USE_SB_DIAGS
#define SB_DIAG_UNUSED __attribute__((warn_unused_result))
#else
#define SB_DIAG_UNUSED
#endif

#ifdef USE_SB_DEPRECATED
#define SB_DIAG_DEPRECATED __attribute__((deprecated))
#else
#define SB_DIAG_DEPRECATED
#endif

#endif // !__SB_INT_DIAG_H_
