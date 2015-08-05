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

#ifndef __SB_MSIC_H_
#define __SB_MSIC_H_

#include "msi.h" // MS_Md_Type

extern void ms_interceptor(const char *pp_where,
                           MS_Md_Type *pp_md,
                           int         pv_reqid,
                           bool       *pp_reply,
                           short      *pp_fserr,
                           char       *pp_err);
extern void ms_interceptor_shutdown();

#endif // !__SB_MSIC_H_
