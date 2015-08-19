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

#ifndef TMLIBGLOBAL_H_
#define TMLIBGLOBAL_H_

#include "seabed/trace.h"

// Externals
extern const char *ms_getenv_str(const char *pp_key);
extern void ms_getenv_bool(const char *pp_key, bool *pp_val);
extern const char *ms_getenv_int(const char *pp_key, int *pp_val);

bool  tmlib_trace_enabled(int pv_level);
/* Tracing levels:
0: off
1 : errors and asserts
2 : procedure entry/exit
3 : detailed
4 : set/get and increase/decrease methods
*/

#define TMlibTrace(a, pv_level) \
               {if (tmlib_trace_enabled(pv_level)) \
                trace_printf a;}

#endif
