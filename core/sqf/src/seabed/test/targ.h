//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef __TARG_H_
#define __TARG_H_

#ifndef __TUTILP_H_
#error "include 'tutilp.h' instead"
#endif

// Test argument types
typedef enum {
   TA_Bool = 1,  // bool
   TA_Ign  = 2,  // ignore
   TA_Int  = 3,  // int
   TA_Next = 4,  // next
   TA_Shrt = 5,  // short
   TA_Str  = 6,  // string
   TA_End  = 7   // end
} TAT;
enum { TA_NOMAX = -1 };
// Test string descriptor
typedef struct {
    const char *ip_str;
    int         iv_str_val;
} TSD;
// Test argument descriptor
typedef struct {
    const char  *ip_arg_str;
    TAT          iv_arg_type;
    int          iv_arg_max;
    void        *ip_arg_ref;
} TAD;
typedef struct {
    const char  *ip_arg_str;
    TAT          iv_arg_type;
    int          iv_arg_max;
    void        *ip_arg_ref;
    TSD         *ipp_arg_strs;
} TAD2;

extern void arg_proc_args(TAD    args[],
                          bool   inv_arg_ok,
                          int    argc,
                          char **argv);
// allows more argument data
extern void arg_proc_args2(TAD2   args[],
                           bool   inv_arg_ok,
                           int    argc,
                           char **argv);
extern bool arg_proc_str(TSD         strs[],
                         const char *str,
                         int        *str_val);

#endif // !__TARG_H_
