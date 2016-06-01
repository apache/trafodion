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

#ifndef __SB_FSTRACE_H_
#define __SB_FSTRACE_H_

#include "seabed/int/opts.h"

// globals
extern bool        gv_fs_trace;
extern bool        gv_fs_trace_data;
extern int         gv_fs_trace_data_max;
extern bool        gv_fs_trace_detail;
extern bool        gv_fs_trace_enable;
extern bool        gv_fs_trace_errors;
extern bool        gv_fs_trace_file_delta;
extern int         gv_fs_trace_file_fb;
extern long long   gv_fs_trace_file_maxsize;
extern bool        gv_fs_trace_file_nolock;
extern int         gv_fs_trace_file_unique;
extern bool        gv_fs_trace_mon;
extern bool        gv_fs_trace_mt;
extern bool        gv_fs_trace_params;
extern bool        gv_fs_trace_params0;
extern bool        gv_fs_trace_verbose;

// "C" for dlsym
extern "C" void fs_trace_change_list(const char *pp_key, const char *pp_value);

#endif // !__SB_FSTRACE_H_
