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

#ifndef __SB_UTIL_H_
#define __SB_UTIL_H_

#include "seabed/int/assert.h"
#include "seabed/int/conv.h"
#include "seabed/int/time.h"
#include "seabed/excep.h"

// internal header file

#ifndef sbmin
#define sbmin(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef sbmax
#define sbmax(a,b) ((a) > (b) ? (a) : (b))
#endif

extern void  SB_util_abort(const char *pp_msg);
extern void  SB_util_fatal(const char *pp_msg, bool pv_stderr) SB_THROWS_FATAL;
extern void  SB_util_get_case_insensitive_name(char *pp_inname, char * pp_outname);
extern char *SB_util_get_cmdline(int   pv_pid,
                                 bool  pv_args,
                                 char *pp_cmdline,
                                 int   pv_len);
extern void  SB_util_get_exe(char *pp_exe, int pv_exe_max, bool pv_base);
extern char *SB_util_itoa_int(char *pp_str, unsigned int pv_num, int pv_base);
extern char *SB_util_itoa_ptr(char *pp_str, void *pp_ptr);

extern void  SB_util_short_lock();
extern void  SB_util_short_unlock();

extern void sb_util_write_log(char *pp_buf);

// make printf happy
#  define pfp(p)  (void *) (long) p
#  define pflx(x) (long)(x)
#if __WORDSIZE == 64
#  define PFSIZEOF "%ld"
#  define PFSSIZE  "%ld"
#else
#  define PFSIZEOF "%u"
#  define PFSSIZE  "%u"
#endif

#define SB_CB_TO_PTR(cb) reinterpret_cast<void *>(reinterpret_cast<long>(cb))

#endif // !__SB_UTIL_H_
